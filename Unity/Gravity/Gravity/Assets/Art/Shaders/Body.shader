Shader "Gravity/Body"
{
    Properties
    {
        _Color ("Base Color", Color) = (0.55, 0.62, 0.78, 1)
        _DetailColor ("Detail Color", Color) = (0.28, 0.33, 0.46, 1)
        [HDR] _EmissionColor ("Emission Color", Color) = (0, 0, 0, 1)
        [HDR] _RimColor ("Rim Color", Color) = (0.35, 0.75, 1, 1)

        _EmissionStrength ("Emission Strength", Float) = 0
        _RimStrength ("Rim Strength", Float) = 0
        _DetailStrength ("Surface Detail", Range(0, 1)) = 0.35
        _BandStrength ("Banding", Range(0, 1)) = 0

        _RimPower ("Rim Falloff", Range(0.5, 12)) = 3
        _DetailScale ("Detail Scale", Float) = 6
        _BandScale ("Band Scale", Float) = 26
        _BandWarp ("Band Warp", Float) = 4

        _ShimmerSpeed ("Shimmer Speed", Float) = 0.15
        _ShimmerScale ("Shimmer Scale", Float) = 0.6
        _PlasmaRange ("Plasma Range (min, max)", Vector) = (0.6, 1.3, 0, 0)

        _NoiseOctaveWeight ("Noise Octave Weight", Range(0, 1)) = 0.65
        _NoiseLacunarity ("Noise Lacunarity", Float) = 2.17

        _Glossiness ("Smoothness", Range(0, 1)) = 0.2
        _Metallic ("Metallic", Range(0, 1)) = 0
    }

    SubShader
    {
        Tags { "RenderType" = "Opaque" }
        LOD 250

        CGPROGRAM
        #pragma surface surf Standard fullforwardshadows vertex:vert
        #pragma multi_compile_instancing
        #pragma target 3.0

        static const float HashScale = 0.3183099;
        static const float3 HashOffset = float3(0.71, 0.113, 0.419);
        static const float HashMultiplier = 17.0;

        struct Input
        {
            float3 viewDir;
            float3 objectPos;
        };

        half _RimPower;
        half _DetailScale;
        half _BandScale;
        half _BandWarp;
        half _ShimmerSpeed;
        half _ShimmerScale;
        float4 _PlasmaRange;
        half _NoiseOctaveWeight;
        half _NoiseLacunarity;
        half _Glossiness;
        half _Metallic;

        UNITY_INSTANCING_BUFFER_START(Props)
            UNITY_DEFINE_INSTANCED_PROP(float4, _Color)
            UNITY_DEFINE_INSTANCED_PROP(float4, _DetailColor)
            UNITY_DEFINE_INSTANCED_PROP(float4, _EmissionColor)
            UNITY_DEFINE_INSTANCED_PROP(float4, _RimColor)
            UNITY_DEFINE_INSTANCED_PROP(float, _EmissionStrength)
            UNITY_DEFINE_INSTANCED_PROP(float, _RimStrength)
            UNITY_DEFINE_INSTANCED_PROP(float, _DetailStrength)
            UNITY_DEFINE_INSTANCED_PROP(float, _BandStrength)
        UNITY_INSTANCING_BUFFER_END(Props)

        float Hash31(float3 p)
        {
            p = frac(p * HashScale + HashOffset);
            p *= HashMultiplier;
            return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
        }

        float ValueNoise(float3 x)
        {
            float3 i = floor(x);
            float3 f = frac(x);
            f = f * f * (3.0 - 2.0 * f);

            float n000 = Hash31(i + float3(0, 0, 0));
            float n100 = Hash31(i + float3(1, 0, 0));
            float n010 = Hash31(i + float3(0, 1, 0));
            float n110 = Hash31(i + float3(1, 1, 0));
            float n001 = Hash31(i + float3(0, 0, 1));
            float n101 = Hash31(i + float3(1, 0, 1));
            float n011 = Hash31(i + float3(0, 1, 1));
            float n111 = Hash31(i + float3(1, 1, 1));

            float x00 = lerp(n000, n100, f.x);
            float x10 = lerp(n010, n110, f.x);
            float x01 = lerp(n001, n101, f.x);
            float x11 = lerp(n011, n111, f.x);

            return lerp(lerp(x00, x10, f.y), lerp(x01, x11, f.y), f.z);
        }

        float Fbm(float3 p)
        {
            float primary = ValueNoise(p) * _NoiseOctaveWeight;
            float secondary = ValueNoise(p * _NoiseLacunarity) * (1.0 - _NoiseOctaveWeight);
            return primary + secondary;
        }

        void vert(inout appdata_full v, out Input o)
        {
            UNITY_INITIALIZE_OUTPUT(Input, o);
            o.objectPos = v.vertex.xyz;
        }

        void surf(Input IN, inout SurfaceOutputStandard o)
        {
            float4 baseColor = UNITY_ACCESS_INSTANCED_PROP(Props, _Color);
            float4 detailColor = UNITY_ACCESS_INSTANCED_PROP(Props, _DetailColor);
            float detailStrength = UNITY_ACCESS_INSTANCED_PROP(Props, _DetailStrength);
            float bandStrength = UNITY_ACCESS_INSTANCED_PROP(Props, _BandStrength);

            float3 samplePos = IN.objectPos * _DetailScale;
            float mottle = Fbm(samplePos);

            float bands = sin(IN.objectPos.y * _BandScale + mottle * _BandWarp) * 0.5 + 0.5;
            float mask = saturate(lerp(mottle, bands, bandStrength) * max(detailStrength, bandStrength));

            o.Albedo = lerp(baseColor.rgb, detailColor.rgb, mask);
            o.Metallic = _Metallic;
            o.Smoothness = _Glossiness;

            float4 emissionColor = UNITY_ACCESS_INSTANCED_PROP(Props, _EmissionColor);
            float emissionStrength = UNITY_ACCESS_INSTANCED_PROP(Props, _EmissionStrength);
            float4 rimColor = UNITY_ACCESS_INSTANCED_PROP(Props, _RimColor);
            float rimStrength = UNITY_ACCESS_INSTANCED_PROP(Props, _RimStrength);

            float shimmer = Fbm(samplePos * _ShimmerScale + float3(0, _Time.y * _ShimmerSpeed, 0));
            float plasma = lerp(_PlasmaRange.x, _PlasmaRange.y, shimmer);

            float rim = 1.0 - saturate(dot(normalize(IN.viewDir), o.Normal));
            rim = pow(rim, _RimPower);

            o.Emission = emissionColor.rgb * emissionStrength * plasma
                       + rimColor.rgb * rim * rimStrength;
        }
        ENDCG
    }

    FallBack "Diffuse"
}
