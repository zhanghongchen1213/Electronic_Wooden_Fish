package top.zhcmqtt.ewf.backend;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

/**
 * 电子木鱼 backend 启动类。
 * 基包 {@code top.zhcmqtt.ewf.backend} 使组件扫描覆盖 {@code controller} 与 {@code common}；
 * 单实例单进程、JSON 零库，不引入任何数据库或第二套并行 backend。
 */
@SpringBootApplication
public class EwfBackendApplication {

    public static void main(String[] args) {
        SpringApplication.run(EwfBackendApplication.class, args);
    }
}
