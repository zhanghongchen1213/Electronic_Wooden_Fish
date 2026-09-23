package top.zhcmqtt.ewf.backend.client;

import com.fasterxml.jackson.annotation.JsonProperty;

/** 微信 code2Session 的最小响应映射；session_key 只在 client 内短暂存在。 */
public class Code2SessionResponse {

    private String openid;
    @JsonProperty("session_key")
    private String sessionKey;
    private Integer errcode;
    private String errmsg;

    public String getOpenid() {
        return openid;
    }

    public void setOpenid(String openid) {
        this.openid = openid;
    }

    public String getSessionKey() {
        return sessionKey;
    }

    public void setSessionKey(String sessionKey) {
        this.sessionKey = sessionKey;
    }

    public Integer getErrcode() {
        return errcode;
    }

    public void setErrcode(Integer errcode) {
        this.errcode = errcode;
    }

    public String getErrmsg() {
        return errmsg;
    }

    public void setErrmsg(String errmsg) {
        this.errmsg = errmsg;
    }
}
