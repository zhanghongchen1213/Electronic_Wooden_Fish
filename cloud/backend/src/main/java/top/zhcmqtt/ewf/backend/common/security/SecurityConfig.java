package top.zhcmqtt.ewf.backend.common.security;

import org.springframework.boot.web.servlet.FilterRegistrationBean;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import top.zhcmqtt.ewf.backend.service.IdentityStore;

/** 无 Spring Security 的最小 filter 注册配置。 */
@Configuration
public class SecurityConfig {

    @Bean
    public FilterRegistrationBean<JwtAuthenticationFilter> jwtAuthenticationFilterRegistration(
            JwtTokenProvider tokenProvider, AuthFailureWriter failureWriter, IdentityStore identityStore) {
        FilterRegistrationBean<JwtAuthenticationFilter> registration = new FilterRegistrationBean<>();
        registration.setFilter(new JwtAuthenticationFilter(tokenProvider, failureWriter, identityStore));
        registration.addUrlPatterns("/*");
        registration.setOrder(1);
        registration.setName("ewfJwtAuthenticationFilter");
        return registration;
    }
}
