package top.zhcmqtt.ewf.backend;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import top.zhcmqtt.ewf.backend.support.TestWorkspace;

/**
 * canonical 经文资源的进度分母口径门禁。
 *
 * <p>三端产物同时暴露 {@code counts.totalChars} 与 {@code counts.consumableHan}，二者不可混用：
 * 运行期进度分母的唯一来源是 {@code counts.consumableHan}（可消费汉字数），{@code totalChars}
 * 含标点等非消费字符。本 Story 不引入任何分母，这里只把口径钉死，防止下游（4.6 / Epic 5 / Epic 6）
 * 直接取 {@code totalChars}。
 *
 * <p>断言全部是资源内部关系，不手抄第二份字面量，故资源自身改动即可触发。
 *
 * <p><b>静态校验无法覆盖的边界：</b>本测试只约束该资源文件的字段口径，
 * 不能阻止下游代码用别的常量或自行求和来充当分母。
 */
class ScriptureResourceBaselineTest {

    private static final ObjectMapper OBJECT_MAPPER = new ObjectMapper();

    @Test
    @DisplayName("进度分母唯一来源是 counts.consumableHan，不是 counts.totalChars")
    void 进度分母唯一来源为可消费汉字数() throws IOException {
        JsonNode resource = backendScriptureResource();
        JsonNode counts = resource.path("counts");
        assertTrue(counts.isObject(), "缺少 counts 节点");

        int consumableHan = counts.path("consumableHan").asInt();
        int totalChars = counts.path("totalChars").asInt();
        int sequenceLength = resource.path("consumableSequence").asText().length();
        int stepCount = resource.path("steps").size();

        assertEquals(sequenceLength, consumableHan, "consumableHan 必须等于资源交付的可消费字序列长度");
        assertEquals(stepCount, consumableHan, "consumableHan 必须等于逐步消费序列的步数");
        assertNotEquals(totalChars, consumableHan, "totalChars 含非消费字符，与进度分母不是同一个量");
        assertEquals(totalChars, consumableHan + counts.path("nonConsumable").asInt(),
                "counts 的可消费数与非消费数应恰好构成总字数");

        List<String> countsEqualToConsumable = new ArrayList<>();
        counts.fieldNames().forEachRemaining(name -> {
            if (counts.get(name).asInt() == consumableHan) {
                countsEqualToConsumable.add(name);
            }
        });
        assertEquals(List.of("consumableHan"), countsEqualToConsumable,
                "counts 中只应有 consumableHan 等于可消费字数，避免出现第二个分母候选");
    }

    private static JsonNode backendScriptureResource() throws IOException {
        Path path = TestWorkspace.repoRoot()
                .resolve("cloud/backend/src/main/resources/canonical/heart-sutra.json");
        assertTrue(Files.isRegularFile(path), "缺少 canonical 经文资源：" + path);
        return OBJECT_MAPPER.readTree(Files.readString(path, StandardCharsets.UTF_8));
    }
}
