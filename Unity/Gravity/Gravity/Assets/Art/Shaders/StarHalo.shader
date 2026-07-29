Shader "Gravity/StarHalo"
{
    Properties
    {
        [HDR] _Color ("Color", Color) = (1, 0.85, 0.5, 1)
        _Intensity ("Halo Intensity", Float) = 1
        _CoreIntensity ("Core Intensity", Float) = 1.5
        _Falloff ("Falloff", Range(0.5, 8)) = 2.5
        _CoreFalloffScale ("Core Falloff Scale", Float) = 6
        _Size ("Size Multiplier", Float) = 1
    }

    SubShader
    {
        Tags
        {
            "Queue" = "Transparent"
            "RenderType" = "Transparent"
            "IgnoreProjector" = "True"
            "PreviewType" = "Plane"
        }

        Blend One One
        ZWrite Off
        ZTest LEqual
        Cull Off
        Lighting Off
        Fog { Mode Off }

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #pragma target 2.0

            #include "UnityCG.cginc"

            fixed4 _Color;
            float _Intensity;
            float _CoreIntensity;
            float _Falloff;
            float _CoreFalloffScale;
            float _Size;

            struct appdata_t
            {
                float4 vertex : POSITION;
                float2 texcoord : TEXCOORD0;
            };

            struct v2f
            {
                float4 vertex : SV_POSITION;
                float2 uv : TEXCOORD0;
            };

            v2f vert(appdata_t v)
            {
                v2f o;

                float2 worldScale = float2(
                    length(float3(unity_ObjectToWorld._m00, unity_ObjectToWorld._m10, unity_ObjectToWorld._m20)),
                    length(float3(unity_ObjectToWorld._m01, unity_ObjectToWorld._m11, unity_ObjectToWorld._m21)));

                float3 centerView = UnityObjectToViewPos(float3(0, 0, 0));
                float3 viewPos = centerView + float3(v.vertex.xy * worldScale * _Size, 0);

                o.vertex = mul(UNITY_MATRIX_P, float4(viewPos, 1.0));
                o.uv = v.texcoord;
                return o;
            }

            fixed4 frag(v2f i) : SV_Target
            {
                float2 delta = i.uv * 2.0 - 1.0;
                float radius = saturate(length(delta));
                float edge = saturate(1.0 - radius);

                float halo = pow(edge, _Falloff);
                float core = pow(edge, _Falloff * _CoreFalloffScale);

                half3 col = _Color.rgb * (halo * _Intensity + core * _CoreIntensity);
                return half4(col, halo);
            }
            ENDCG
        }
    }
}
