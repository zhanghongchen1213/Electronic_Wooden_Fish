package top.zhcmqtt.ewf.backend.common.config;

import java.time.Clock;
import java.time.ZoneId;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

/**
 * 可信时间源（Story 5.3 裁决 A）。
 *
 * <p><b>选择：</b>显式提供 {@link Clock} bean，zone 固定为 {@code Asia/Shanghai}，与
 * {@code spring.jackson.time-zone: Asia/Shanghai} 字面一致。
 * <b>理由：</b>Spring Boot 不自动配置 Clock（spring-boot#31397）；日界归档禁止
 * {@code LocalDate.now()} 无参或 {@code ZoneId.systemDefault()}。
 * <b>约束：</b>生产用 {@link Clock#system(ZoneId)}；测试注入 {@link Clock#fixed} / 可拨动实现。
 */
@Configuration
public class ClockConfig {

    /** 与 {@code RuntimeBaselineTest} / {@code application.yml} 钉死的时区字面量一致。 */
    public static final ZoneId ZONE = ZoneId.of("Asia/Shanghai");

    @Bean
    public Clock clock() {
        return Clock.system(ZONE);
    }
}
