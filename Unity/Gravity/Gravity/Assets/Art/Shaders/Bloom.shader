Shader "Hidden/Gravity/Bloom"
{
    Properties
    {
        _MainTex ("Source", 2D) = "white" {}
    }

    CGINCLUDE
    #include "UnityCG.cginc"

    static const float MinBrightness = 0.0001;
    static const float BoxSampleWeight = 0.25;
    static const float TentSampleNormalization = 1.0 / 16.0;
    static const float BoxSampleDelta = 1.0;

    sampler2D _MainTex;
    float4 _MainTex_TexelSize;

    sampler2D _BloomTex;

    float4 _FilterParams;
    float _SampleScale;
    float _Intensity;
    fixed4 _Tint;

    struct v2f
    {
        float4 vertex : SV_POSITION;
        float2 uv : TEXCOORD0;
    };

    v2f VertBlit(appdata_img v)
    {
        v2f o;
        o.vertex = UnityObjectToClipPos(v.vertex);
        o.uv = v.texcoord;
        return o;
    }

    half3 SampleBox(float2 uv, float delta)
    {
        float4 offset = _MainTex_TexelSize.xyxy * float4(-delta, -delta, delta, delta);
        half3 sum = tex2D(_MainTex, uv + offset.xy).rgb
                  + tex2D(_MainTex, uv + offset.zy).rgb
                  + tex2D(_MainTex, uv + offset.xw).rgb
                  + tex2D(_MainTex, uv + offset.zw).rgb;
        return sum * BoxSampleWeight;
    }

    half3 SampleTent(float2 uv, float scale)
    {
        float4 d = _MainTex_TexelSize.xyxy * float4(1, 1, -1, 0) * scale;

        half3 sum = tex2D(_MainTex, uv - d.xy).rgb;
        sum += tex2D(_MainTex, uv - d.wy).rgb * 2;
        sum += tex2D(_MainTex, uv - d.zy).rgb;

        sum += tex2D(_MainTex, uv + d.zw).rgb * 2;
        sum += tex2D(_MainTex, uv).rgb * 4;
        sum += tex2D(_MainTex, uv + d.xw).rgb * 2;

        sum += tex2D(_MainTex, uv + d.zy).rgb;
        sum += tex2D(_MainTex, uv + d.wy).rgb * 2;
        sum += tex2D(_MainTex, uv + d.xy).rgb;

        return sum * TentSampleNormalization;
    }

    half3 ApplyThreshold(half3 color)
    {
        half brightness = max(color.r, max(color.g, color.b));

        half soft = brightness - _FilterParams.y;
        soft = clamp(soft, 0, _FilterParams.z);
        soft = soft * soft * _FilterParams.w;

        half contribution = max(soft, brightness - _FilterParams.x);
        contribution /= max(brightness, MinBrightness);

        return color * contribution;
    }
    ENDCG

    SubShader
    {
        Cull Off
        ZWrite Off
        ZTest Always

        Pass
        {
            CGPROGRAM
            #pragma vertex VertBlit
            #pragma fragment frag
            #pragma target 3.0

            half4 frag(v2f i) : SV_Target
            {
                half3 color = SampleBox(i.uv, BoxSampleDelta);
                return half4(ApplyThreshold(color), 1.0);
            }
            ENDCG
        }

        Pass
        {
            CGPROGRAM
            #pragma vertex VertBlit
            #pragma fragment frag
            #pragma target 3.0

            half4 frag(v2f i) : SV_Target
            {
                return half4(SampleBox(i.uv, BoxSampleDelta), 1.0);
            }
            ENDCG
        }

        Pass
        {
            Blend One One

            CGPROGRAM
            #pragma vertex VertBlit
            #pragma fragment frag
            #pragma target 3.0

            half4 frag(v2f i) : SV_Target
            {
                return half4(SampleTent(i.uv, _SampleScale), 1.0);
            }
            ENDCG
        }

        Pass
        {
            CGPROGRAM
            #pragma vertex VertBlit
            #pragma fragment frag
            #pragma target 3.0

            half4 frag(v2f i) : SV_Target
            {
                half4 source = tex2D(_MainTex, i.uv);
                half3 bloom = tex2D(_BloomTex, i.uv).rgb * _Intensity * _Tint.rgb;
                return half4(source.rgb + bloom, source.a);
            }
            ENDCG
        }
    }

    FallBack Off
}
