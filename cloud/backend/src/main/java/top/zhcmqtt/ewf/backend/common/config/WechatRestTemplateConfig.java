package top.zhcmqtt.ewf.backend.common.config;

import java.net.Proxy;
import java.net.ProxySelector;
import java.net.URI;
import java.net.http.HttpClient.Version;
import java.net.http.HttpClient;
import java.time.Duration;
import java.util.List;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.client.JdkClientHttpRequestFactory;
import org.springframework.web.client.RestTemplate;

/** 微信直连 HTTP 客户端：禁用系统代理并固定超时，避免误走支付/其它 client。 */
@Configuration
public class WechatRestTemplateConfig {

    @Bean("wechatDirectRestTemplate")
    public RestTemplate wechatDirectRestTemplate() {
        HttpClient client = HttpClient.newBuilder()
                .proxy(new NoSystemProxySelector())
                .version(Version.HTTP_1_1)
                .connectTimeout(Duration.ofSeconds(10))
                .build();
        JdkClientHttpRequestFactory requestFactory = new JdkClientHttpRequestFactory(client);
        requestFactory.setReadTimeout(Duration.ofSeconds(15));
        return new RestTemplate(requestFactory);
    }

    /** JDK HttpClient 没有 no-proxy 常量，显式返回 NO_PROXY 防止读取环境代理。 */
    static final class NoSystemProxySelector extends ProxySelector {
        @Override
        public List<Proxy> select(URI uri) {
            return List.of(Proxy.NO_PROXY);
        }

        @Override
        public void connectFailed(URI uri, java.net.SocketAddress sa, java.io.IOException ioe) {
            // 直连失败由 RestTemplate/WechatMiniClient 转换为统一上游错误。
        }
    }
}
