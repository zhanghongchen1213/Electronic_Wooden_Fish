package top.zhcmqtt.ewf.backend.service;

import java.time.Clock;
import java.time.LocalDate;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.Iterator;
import java.util.Map;
import java.util.Optional;

import org.springframework.stereotype.Service;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;

import top.zhcmqtt.ewf.backend.common.config.ClockConfig;
import top.zhcmqtt.ewf.backend.common.exception.BusinessException;
import top.zhcmqtt.ewf.backend.dto.sync.HistoryStatsResponse;

/**
 * 确认差量日桶归档、catch-up 与历史统计聚合的唯一服务（Story 5.3）。
 *
 * <p>控制器不得散落日期算法；本类是确认差量 → 今日桶、桶合计 vs {@code acked_total} catch-up、
 * 聚合查询的单一入口。
 *
 * <h2>裁决 A：归档触发与时间源</h2>
 * <p><b>选择：</b>仅当权威 {@code acked_total} 相对「上次成功收敛的桶合计」增大时，把差值
 * {@code delta} 全部记入 {@code LocalDate.now(clock)}（zone={@code Asia/Shanghai}）对应桶的
 * {@code confirmed_taps}。时间源唯一：注入的 {@link Clock}。
 * <b>理由：</b>离线差量按确认日归档（AD-6）；设备不保存绝对敲击时间；请求体无日期字段。
 * <b>约束：</b>禁止读取请求日期、设备固件时间或 {@link java.time.ZoneId#systemDefault()}；
 * 禁止 {@code LocalDate.now()} 无参。
 *
 * <h2>裁决 B：桶 JSON 形状（实现冻结，非契约字段）</h2>
 * <p><b>选择：</b>{@code buckets} 键 = 上海自然日 {@code yyyy-MM-dd}；桶值仅
 * {@code confirmed_taps}（非负 int）。无敲击日期不写 0 桶。顶层信封仍恰为
 * {@code {schema_version, buckets}}。
 * <b>约束：</b>{@link DailyStatsStore#allowedFields()} 继续空集；不得改契约 §11 字段列。
 *
 * <h2>裁决 E：派生失败与 catch-up</h2>
 * <p><b>选择：</b>不变量目标 {@code Σ confirmed_taps == acked_total}。写路径（含 no-op /
 * {@code action_id} 短路径）在持有 {@code deviceLocks} 时：若 {@code acked_total > Σ}，差额记入
 * <b>当日</b>桶；若 {@code Σ > acked_total} → {@code 50000} fail closed。日统计写失败吞掉、不回滚权威。
 * <b>约束：</b>查询路径只读，发现不一致时不写盘。
 *
 * <h2>裁决 F：连续天数</h2>
 * <p><b>选择：</b>从「今日」起向过去逐日查看桶：若今日 {@code confirmed_taps==0}，则从昨日开始计；
 * 连续有敲击的天数即 {@code streak_days}；若今日与昨日皆 0 → {@code 0}。
 */
@Service
public class HistoryStatsService {

    /** 桶值字段名（实现冻结，非契约 §11 字段列）。 */
    public static final String CONFIRMED_TAPS = "confirmed_taps";

    private static final DateTimeFormatter DAY_KEY = DateTimeFormatter.ISO_LOCAL_DATE;

    private final DailyStatsStore dailyStatsStore;
    private final ProgressStore progressStore;
    private final ObjectMapper objectMapper;
    private final Clock clock;

    public HistoryStatsService(DailyStatsStore dailyStatsStore, ProgressStore progressStore,
            ObjectMapper objectMapper, Clock clock) {
        this.dailyStatsStore = dailyStatsStore;
        this.progressStore = progressStore;
        this.objectMapper = objectMapper;
        this.clock = clock;
        if (!ClockConfig.ZONE.equals(clock.getZone())) {
            throw new IllegalStateException("HistoryStatsService Clock 时区必须为 Asia/Shanghai，实际="
                    + clock.getZone());
        }
    }

    /**
     * 写路径归档 / catch-up：在权威 {@code progress.json} 已写入之后调用。
     *
     * <p>写失败（IO）吞掉以保持「派生失败不回滚权威」；桶合计大于 {@code ackedTotal} 时抛
     * {@code 50000} 且不静默砍桶。读损坏/格式无效时按空桶重建（AC#3 派生面 catch-up），
     * 不阻断后续写路径收敛。
     */
    public void archiveAndCatchUp(int ackedTotal) {
        if (ackedTotal < 0) {
            throw BusinessException.serverError("已确认累计不可为负");
        }
        ObjectNode buckets = readBucketsForWrite();
        long sum = sumConfirmed(buckets);
        if (sum > ackedTotal) {
            throw BusinessException.serverError("日统计桶合计超过已确认累计，数据可能遭篡改");
        }
        if (sum == ackedTotal) {
            return;
        }
        int delta = ackedTotal - (int) sum;
        String todayKey = LocalDate.now(clock).format(DAY_KEY);
        int current = 0;
        JsonNode existing = buckets.get(todayKey);
        if (existing != null && existing.isObject()) {
            current = readConfirmedTaps((ObjectNode) existing);
        }
        // 裁决 B：桶值仅 confirmed_taps——写回时新建 object，剥离其它键。
        ObjectNode dayBucket = objectMapper.createObjectNode();
        dayBucket.put(CONFIRMED_TAPS, current + delta);
        buckets.set(todayKey, dayBucket);
        try {
            dailyStatsStore.write(buckets);
        } catch (RuntimeException ignored) {
            // 派生写失败不回滚权威；同 action_id / 再次 report 可 catch-up 收敛。
        }
    }

    /**
     * 写路径读取：缺失 → 空桶；损坏/不可读/格式无效 → 空桶以便 catch-up 重建。
     * 查询路径仍走 {@link DailyStatsStore#read()} 的 fail-closed。
     */
    private ObjectNode readBucketsForWrite() {
        try {
            return dailyStatsStore.read()
                    .map(ObjectNode::deepCopy)
                    .orElseGet(objectMapper::createObjectNode);
        } catch (BusinessException ex) {
            return objectMapper.createObjectNode();
        }
    }

    /**
     * 只读聚合查询（裁决 C/D/F）。不得创建目录、写文件或 catch-up 写盘。
     */
    public HistoryStatsResponse query() {
        Optional<ObjectNode> progressOpt = progressStore.read();
        int ackedTotal = progressOpt.map(node -> intOrZero(node, "acked_total")).orElse(0);
        int localTotal = progressOpt.map(node -> intOrZero(node, "local_total")).orElse(0);

        Optional<ObjectNode> bucketsOpt = dailyStatsStore.read();
        ObjectNode buckets = bucketsOpt.orElseGet(objectMapper::createObjectNode);
        LocalDate today = LocalDate.now(clock);

        int todayTaps = tapsOn(buckets, today);
        int last7 = sumWindow(buckets, today, 7);
        int last30 = sumWindow(buckets, today, 30);
        int streak = streakDays(buckets, today);
        long bucketSum = sumConfirmed(buckets);

        boolean empty = ackedTotal == 0 && bucketSum == 0;
        boolean pendingSync = localTotal > ackedTotal;

        return new HistoryStatsResponse(todayTaps, last7, last30, ackedTotal, streak, empty, pendingSync);
    }

    /** 桶合计（供测试与写路径诊断）。仅计入合法 {@code yyyy-MM-dd} 键。 */
    public long sumConfirmed(ObjectNode buckets) {
        if (buckets == null || buckets.isEmpty()) {
            return 0L;
        }
        long sum = 0L;
        Iterator<Map.Entry<String, JsonNode>> fields = buckets.fields();
        while (fields.hasNext()) {
            Map.Entry<String, JsonNode> entry = fields.next();
            if (!isDayKey(entry.getKey())) {
                continue;
            }
            JsonNode value = entry.getValue();
            if (value != null && value.isObject()) {
                sum += readConfirmedTaps((ObjectNode) value);
            }
        }
        return sum;
    }

    private static boolean isDayKey(String key) {
        if (key == null || key.isBlank()) {
            return false;
        }
        try {
            LocalDate.parse(key, DAY_KEY);
            return true;
        } catch (DateTimeParseException ex) {
            return false;
        }
    }

    private int streakDays(ObjectNode buckets, LocalDate today) {
        LocalDate cursor = today;
        if (tapsOn(buckets, today) == 0) {
            cursor = today.minusDays(1);
            if (tapsOn(buckets, cursor) == 0) {
                return 0;
            }
        }
        int streak = 0;
        while (tapsOn(buckets, cursor) > 0) {
            streak++;
            cursor = cursor.minusDays(1);
        }
        return streak;
    }

    private int sumWindow(ObjectNode buckets, LocalDate today, int daysInclusive) {
        int sum = 0;
        for (int offset = 0; offset < daysInclusive; offset++) {
            sum += tapsOn(buckets, today.minusDays(offset));
        }
        return sum;
    }

    private int tapsOn(ObjectNode buckets, LocalDate day) {
        JsonNode node = buckets.get(day.format(DAY_KEY));
        if (node == null || !node.isObject()) {
            return 0;
        }
        return readConfirmedTaps((ObjectNode) node);
    }

    private static int readConfirmedTaps(ObjectNode bucket) {
        JsonNode value = bucket.get(CONFIRMED_TAPS);
        if (value == null || !value.isIntegralNumber()) {
            return 0;
        }
        int taps = value.asInt();
        return Math.max(0, taps);
    }

    private static int intOrZero(ObjectNode node, String field) {
        JsonNode value = node.get(field);
        if (value == null || !value.isIntegralNumber()) {
            return 0;
        }
        return value.asInt();
    }
}
