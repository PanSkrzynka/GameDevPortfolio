Shader "Hidden/RewindEffect"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
        _Strength ("Strength", Range(0,1)) = 0
        _Phase ("Phase", Float) = 0
    }

    SubShader
    {
        Cull Off
        ZWrite Off
        ZTest Always

        Pass
        {
            CGPROGRAM
            #pragma vertex vert_img
            #pragma fragment frag
            #include "UnityCG.cginc"

            sampler2D _MainTex;
            float _Strength;
            float _Phase;

            float Hash(float n)
            {
                return frac(sin(n * 12.9898 + 78.233) * 43758.5453);
            }

            fixed4 frag(v2f_img i) : SV_Target
            {
                float s = saturate(_Strength);
                float2 uv = i.uv;

                float band = floor(uv.y * 40.0);
                float jitter = (Hash(band + floor(_Phase * 18.0)) - 0.5) * 0.011 * s;
                uv.x += jitter;

                float roll = frac(uv.y + _Phase * 0.4);
                float bar = smoothstep(0.0, 0.05, roll) * (1.0 - smoothstep(0.09, 0.16, roll));
                uv.x += bar * 0.009 * s;

                uv = saturate(uv);

                float ca = 0.0045 * s;
                fixed4 col;
                col.r = tex2D(_MainTex, float2(saturate(uv.x + ca), uv.y)).r;
                col.g = tex2D(_MainTex, uv).g;
                col.b = tex2D(_MainTex, float2(saturate(uv.x - ca), uv.y)).b;
                col.a = 1.0;

                float lum = dot(col.rgb, float3(0.299, 0.587, 0.114));
                float3 tinted = lum * float3(0.52, 0.86, 1.10);
                col.rgb = lerp(col.rgb, tinted, s * 0.85);

                float scan = 0.88 + 0.12 * sin(uv.y * _ScreenParams.y * 1.4);
                col.rgb *= lerp(1.0, scan, s);

                col.rgb += bar * 0.14 * s;

                float2 d = uv - 0.5;
                col.rgb *= 1.0 - dot(d, d) * 1.5 * s;

                return col;
            }
            ENDCG
        }
    }

    Fallback Off
}
