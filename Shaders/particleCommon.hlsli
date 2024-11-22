struct Particle
{
    float3 PositionW;
    float Size;
    
    float3 Velocity;
    float Rotation;
    
    float4 StartColor;
    float4 Color;
    
    float3 EmitterPosition;
    float GravityModifier;

    float LifeTime;
    float Age;
};

struct ParticleInstance
{
    matrix Transform;
    float TimePos;
    float FrameTime;
    int NumToEmit;
    float RandomSeed;
};

struct ParticleMain
{
    float4 StartColor[2];

    float2 StartLifeTime;
    float2 StartSpeed;

    float2 StartRotation;
    float2 StartSize;

    float2 GravityModifier;
    float SimulationSpeed;
    int MaxParticleSize;
};

struct ParticleShape
{
    matrix Transform;
    float3 Position;
    float pad0; // 16바이트 정렬을 위한 패딩
    float3 Rotation;
    float pad1; // 16바이트 정렬을 위한 패딩
    float3 Scale;
    float pad2; // 16바이트 정렬을 위한 패딩

    int ShapeType;
    int ModeType;
    float Angle;
    float Radius;

    float DonutRadius;
    float Arc;
    float Spread;

    float RadiusThickness;
};

struct ParticleVelocityOverLifetime
{
    float3 Velocity;
    float pad4; // 16바이트 정렬을 위한 패딩
    float3 Orbital;
    float pad5; // 16바이트 정렬을 위한 패딩
    float3 Offset;
    float pad6; // 16바이트 정렬을 위한 패딩
    int IsUsed;
    float3 pad7; // 16바이트 정렬을 위한 패딩
};

struct ParticleLimitVelocityOverLifetime
{
    float Speed;
    float Dampen;
    int IsUsed;
    float pad8; // 16바이트 정렬을 위한 패딩
};

struct ParticleForceOverLifeTime
{
    float3 Force;
    int IsUsed;
};

struct ParticleColorOverLifetime
{
    float4 ColorRatios[8];
    float4 AlphaRatios[8];
    uint AlphaRatioCount;
    uint ColorRatioCount;
    int IsUsed;
    float pad9; // 16바이트 정렬을 위한 패딩
};

struct ParticleSizeOverLifetime
{
    float2 PointA;
    float2 PointB;
    float2 PointC;
    float2 PointD;
    int IsUsed;
    float3 pad10; // 16바이트 정렬을 위한 패딩
};

struct ParticleRotationOverLifetime
{
    float AngularVelocity;
    int IsUsed;
    float2 pad11; // 16바이트 정렬을 위한 패딩
};

struct ParticleRender
{
    float4 BaseColor;
    float4 EmissiveColor;
    float4x4 TexTransform;
    
    int RenderMode;
    int ColorMode;
    int UseAlbedo;
    int UseEmissive;
    
    int UseMultiplyAlpha;
    float AlphaCutOff;
};

cbuffer cbParticleFrame
{
    matrix gViewMatrix;
    matrix gProjMatrix;
    matrix gInvProjMatrix;
    uint gScreenWidth;
    uint gScreenHeight;
    float2 padding;
};

cbuffer cbParticleObject
{
    ParticleInstance gParticleInstance;
    ParticleMain gParticleMain;
    ParticleShape gParticleShape;
    ParticleVelocityOverLifetime gParticleVelocityOverLifeTime;
    ParticleLimitVelocityOverLifetime gParticleLimitVelocityOverLifeTime;
    ParticleForceOverLifeTime gParticleForceOverLifeTime;
    ParticleColorOverLifetime gParticleColorOverLifeTime;
    ParticleSizeOverLifetime gParticleSizeOverLifeTime;
    ParticleRotationOverLifetime gParticleRotationOverLifeTime;
    ParticleRender gParticleRender;
};

#define SHAPE_SPHERE 0
#define SHAPE_HEMISPHERE 1
#define SHAPE_CONE 2
#define SHAPE_DONUT 3
#define SHAPE_BOX 4
#define SHAPE_CIRCLE 5
#define SHAPE_RECTANGLE 6
static const float PI = 3.141592;

void processShape(float4 random, out float3 posW, out float3 velocity)
{
    float speed = lerp(gParticleMain.StartSpeed[0], gParticleMain.StartSpeed[1], (random.x + 1) * 0.5f);
    matrix finalTransform = mul(gParticleShape.Transform, gParticleInstance.Transform);

    posW = float3(gParticleInstance.Transform[3][0], gParticleInstance.Transform[3][1], gParticleInstance.Transform[3][2]); /*gCommonInfo.gEmitPosW.xyz;*/
    velocity = speed * normalize(float3(finalTransform[1][0], finalTransform[1][1], finalTransform[1][2])); /* gCommonInfo.gEmitDirW;*/
    
    if (gParticleShape.ShapeType == SHAPE_SPHERE)
    {
        float3 nRandom = normalize(random.xyz); // xz 평면 기준으로 Arc를 적용시키자.
        float _length = length(nRandom.xz); // 벡터의 길이를 구한다.
        float cosTheata = nRandom.xz.x / _length; // 벡터의 길이 = 빗변, \\  밑면 / 빗변 = cosTheata
        float _radian = nRandom.z > 0 ? acos(cosTheata) : 2 * PI - acos(cosTheata); // 파티클의 방출 각도. 
            
        _radian = _radian * (gParticleShape.Arc / (2 * PI)); // 변환.
        nRandom.x = cos(_radian) * _length;
        nRandom.z = sin(_radian) * _length;

        velocity = speed * nRandom;
        
        float posLength = length(nRandom * gParticleShape.Radius);
        float radiusThickness = saturate(gParticleShape.RadiusThickness);
        posLength = radiusThickness * posLength + (1.0f - radiusThickness) * saturate((random.w + 1.f) * 0.5f) * posLength;
        
        float3 pos = nRandom * posLength;
        posW = mul(float4(pos, 1), finalTransform).xyz;
    }
    else if (gParticleShape.ShapeType == SHAPE_HEMISPHERE)
    {
        float3 nRandom = normalize(random.xyz);
        float _length = length(nRandom.xz); // 벡터의 길이를 구한다.
        float cosTheata = nRandom.xz.x / _length; // 벡터의 길이 = 빗변, \\  밑면 / 빗변 = cosTheata
        float _radian = nRandom.z > 0 ? acos(cosTheata) : 2 * PI - acos(cosTheata); // 파티클의 방출 각도. 
         
        _radian = _radian * (gParticleShape.Arc / (2 * PI)); // 변환.
        nRandom.x = cos(_radian) * _length;
        nRandom.z = sin(_radian) * _length;

        velocity = speed * float3(nRandom.x, abs(nRandom.y), nRandom.z);
			
        float posLength = length(nRandom * gParticleShape.Radius);
        float radiusThickness = saturate(gParticleShape.RadiusThickness);
        posLength = radiusThickness * posLength + (1.0f - radiusThickness) * saturate((random.w + 1.f) * 0.5f) * posLength;
        
        float3 pos = nRandom * posLength;
        posW = mul(float4(pos.x, abs(pos.y), pos.z, 1), finalTransform).xyz;
    }
    else if (gParticleShape.ShapeType == SHAPE_CONE)
    {
        float a = 1 / cos(gParticleShape.Angle); // 빗변 높이가 1이라고 가정.
        float y = abs(sin(gParticleShape.Angle) * a); // 밑변의 길이.

        float TopRadius = (y + gParticleShape.Radius);
            
        TopRadius = TopRadius / gParticleShape.Radius; // 스케일 값으로 사용.
            
        float3 nRandom = normalize(random.xyz); // xz 평면 기준으로 Arc를 적용시키자.
            
        float _length = length(nRandom.xz); // 벡터의 길이를 구한다.
            
        float cosTheata = nRandom.xz.x / _length; // 벡터의 길이 = 빗변, \\  밑면 / 빗변 = cosTheata

        float _radian = nRandom.z > 0 ? acos(cosTheata) : 2 * PI - acos(cosTheata); // 파티클의 방출 각도. 
            
        _radian = _radian * (gParticleShape.Arc / (2 * PI)); // 변환.
            
        nRandom.x = cos(_radian) * _length;
        nRandom.z = sin(_radian) * _length;
            
        float posLength = length(nRandom.xz * gParticleShape.Radius);
        float radiusThickness = saturate(gParticleShape.RadiusThickness);
        posLength = radiusThickness * posLength + (1.0f - radiusThickness) * saturate((random.w + 1.f) * 0.5f) * posLength;
        
        float2 pos = normalize(nRandom.xz) * posLength;
            
        float3 topPosition = mul(float4((float3(pos.x, 0, pos.y) * TopRadius + float3(0, 1, 0)).xyz, 1), finalTransform).xyz;
        posW = mul(float4(float3(pos.x, 0, pos.y), 1), finalTransform).xyz;

        velocity = normalize(topPosition - posW) * speed;
    }
    else if (gParticleShape.ShapeType == SHAPE_DONUT)
    {
        float3 nRandom = normalize(random.xyz); // xz 평면 기준으로 Arc를 적용시키자.
        float _length = length(nRandom.xz); // 벡터의 길이를 구한다.
        float cosTheata = nRandom.xz.x / _length; // 벡터의 길이 = 빗변, \\  밑면 / 빗변 = cosTheata
        float _radian = nRandom.z > 0 ? acos(cosTheata) : 2 * PI - acos(cosTheata); // 파티클의 방출 각도. 
            
        _radian = _radian * (gParticleShape.Arc / (2 * PI)); // 변환.
        nRandom.x = cos(_radian) * _length;
        nRandom.z = sin(_radian) * _length;
            
        // 반지름 벡터
        float2 radiusVec = nRandom.xz;
        radiusVec = normalize(radiusVec);
        float posLength = length(random.xy * gParticleShape.DonutRadius);
       // posLength = gParticleShape.RadiusThickness * gParticleShape.DonutRadius + (1.0f - gParticleShape.RadiusThickness) * posLength;
        float radiusThickness = saturate(gParticleShape.RadiusThickness);
        posLength = radiusThickness * gParticleShape.DonutRadius + (1.0f - radiusThickness) * saturate((random.w + 1.f) * 0.5f) * gParticleShape.DonutRadius;
        
        float3 pos = float3(normalize(random.xy) * posLength, 0.0f);
            
        posW = mul(float4((float3(radiusVec.x, 0, radiusVec.y) * gParticleShape.Radius + pos).xyz, 1), finalTransform).xyz;
        velocity = speed * normalize(random.yzw);
    }
    else if (gParticleShape.ShapeType == SHAPE_BOX)
    {
        posW = mul(float4(random.xyz, 1), finalTransform).xyz;
    }
    else if (gParticleShape.ShapeType == SHAPE_CIRCLE)
    {
        float3 nRandom = normalize(random.xyz); // xz 평면 기준으로 Arc를 적용시키자.
        float _length = length(nRandom.xz); // 벡터의 길이를 구한다.
        float cosTheata = nRandom.xz.x / _length; // 벡터의 길이 = 빗변, \\  밑면 / 빗변 = cosTheata
        float _radian = nRandom.z > 0 ? acos(cosTheata) : 2 * PI - acos(cosTheata); // 파티클의 방출 각도. 
            
        _radian = _radian * (gParticleShape.Arc / (2 * PI)); // 변환.
        nRandom.x = cos(_radian) * _length;
        nRandom.z = sin(_radian) * _length;
            
        float2 radiusVec = nRandom.xz;
        float posLength = length(nRandom.xz * gParticleShape.Radius);
        //posLength = gParticleShape.RadiusThickness * gParticleShape.Radius + (1.0f - gParticleShape.RadiusThickness) * posLength;
        float radiusThickness = saturate(gParticleShape.RadiusThickness);
        posLength = radiusThickness * gParticleShape.Radius + (1.0f - radiusThickness) * saturate((random.w + 1.f) * 0.5f) * gParticleShape.Radius;
       
        float2 pos = normalize(nRandom.xz) * posLength;
        posW = mul(float4(pos.x, 0, pos.y, 1), finalTransform).xyz;
        velocity = normalize(mul(float3(nRandom.x, 0, nRandom.z), (float3x3) finalTransform)) * speed;
    }
    else if (gParticleShape.ShapeType == SHAPE_RECTANGLE)
    {
        posW = mul(float4((float3(random.x, 0, random.z)).xyz, 1), finalTransform).xyz;
    }
}

float4 rotate_angle_axis(float angle, float3 axis)
{
    float sn = sin(angle * 0.5);
    float cs = cos(angle * 0.5);
    return float4(axis * sn, cs);
}

float4 qmul(float4 q1, float4 q2)
{
    return float4(
        q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz),
        q1.w * q2.w - dot(q1.xyz, q2.xyz)
    );
}

float3 rotate_vector(float3 v, float4 r)
{
    float4 r_c = r * float4(-1, -1, -1, 1);
    return qmul(r, qmul(float4(v, 0), r_c)).xyz;
}

void processOrbital(float3 posW, float3 InitEmitterPos, float3 velW, out float3 velW_Out, out float3 posW_Out)
{
    InitEmitterPos += gParticleVelocityOverLifeTime.Offset;
    float3 axis = gParticleVelocityOverLifeTime.Orbital;

    if (length(axis) != 0)
    {
        float3 n_axis = normalize(axis);

        float3 orbitalCenterPos = dot(posW, n_axis) * n_axis + InitEmitterPos; // 

        float3 offset = posW - InitEmitterPos; // 

        float power = length(axis);
        
        float angle = gParticleInstance.FrameTime * power * gParticleMain.SimulationSpeed;;

        float4 rotateQuat = rotate_angle_axis(angle, n_axis);

        float3 rotated = rotate_vector(offset, rotateQuat);

        velW = rotate_vector(velW, rotateQuat);

        posW = rotated + InitEmitterPos;
    }
    
    velW_Out = velW;
    posW_Out = posW;

}

void processVelocityOverLifeTime(float ratio, Particle particleIn, out Particle particelOut)
{
    if (gParticleVelocityOverLifeTime.IsUsed)
    {
        float3 velocity = float3(0, 0, 0);
        
        velocity = lerp(0, gParticleVelocityOverLifeTime.Velocity, ratio);

        particleIn.PositionW += velocity * gParticleInstance.FrameTime * gParticleMain.SimulationSpeed;;

        processOrbital(particleIn.PositionW, particleIn.EmitterPosition, particleIn.Velocity, particleIn.Velocity, particleIn.PositionW);
    }

    particelOut = particleIn;
}

void processLimitVelocityOverLifeTime(float ratio, float3 VelWIn, out float3 VelWOut)
{
    if (gParticleLimitVelocityOverLifeTime.IsUsed)
    {        
        float nowSpeed = length(VelWIn);
        
        if (nowSpeed > gParticleLimitVelocityOverLifeTime.Speed)
        {
            float overSpeed = nowSpeed - gParticleLimitVelocityOverLifeTime.Speed;
            nowSpeed -= overSpeed * gParticleLimitVelocityOverLifeTime.Dampen;
            VelWIn = nowSpeed * normalize(VelWIn);
        }
    }

    VelWOut = VelWIn;
}

void processForceOverLifeTime(float3 velW, out float3 velWOut)
{
    if (gParticleForceOverLifeTime.IsUsed)
    {
        float3 force = float3(0, 0, 0);

        velWOut = velW + gParticleForceOverLifeTime.Force * gParticleInstance.FrameTime * gParticleMain.SimulationSpeed;
    }
    else
    {
        velWOut = velW;
    }

}

void processColorOverLifeTime(float ratio, out float4 color)
{
    if (gParticleColorOverLifeTime.IsUsed)
    {
        
        uint i;
        
        for (i = 0; i < gParticleColorOverLifeTime.AlphaRatioCount; ++i)
        {
            float curRatio = gParticleColorOverLifeTime.AlphaRatios[i].y;
            
            if (curRatio > ratio)
            {
                break;
            }
        }
        
        if (i == gParticleColorOverLifeTime.AlphaRatioCount)
        {
            --i;
        }
        
        float endAlpha = gParticleColorOverLifeTime.AlphaRatios[i].x;
        float startAlpha = 0.f;
        float alphaRatio = 1.f;
                
        if (i != 0)
        {
            float curRatio = gParticleColorOverLifeTime.AlphaRatios[i].y;
            float prevRatio = gParticleColorOverLifeTime.AlphaRatios[i - 1].y;
            
            startAlpha = gParticleColorOverLifeTime.AlphaRatios[i - 1].x;
            alphaRatio = (ratio - prevRatio) / (curRatio - prevRatio);
            alphaRatio = clamp(alphaRatio, 0, 1);
        }
        
        color.a = lerp(startAlpha, endAlpha, alphaRatio);
        
        for (i = 0; i < gParticleColorOverLifeTime.ColorRatioCount; ++i)
        {
            float curRatio = gParticleColorOverLifeTime.ColorRatios[i].w;
            
            if (curRatio > ratio)
            {
                break;
            }
        }
        
        if (i == gParticleColorOverLifeTime.ColorRatioCount)
        {
            --i;
        }
        
        float3 endColor = gParticleColorOverLifeTime.ColorRatios[i].xyz;
        float3 startColor = 0.f;
        float colorRatio = 1.f;
                
        if (i != 0)
        {
            float curRatio = gParticleColorOverLifeTime.ColorRatios[i].w;
            float prevRatio = gParticleColorOverLifeTime.ColorRatios[i - 1].w;
            
            startColor = gParticleColorOverLifeTime.ColorRatios[i - 1].xyz;
            colorRatio = (ratio - prevRatio) / (curRatio - prevRatio);
            
            colorRatio = clamp(colorRatio, 0, 1);
        }
        
        color.xyz = lerp(startColor, endColor, colorRatio);
    }
    else
    {
        color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void processSizeOverLifeTime(float sizeIn, float3 ratio, out float sizeOut)
{
    if (gParticleSizeOverLifeTime.IsUsed)
    {
        float t = ratio.x;
        float s = 1.0f - t;

        float2 sizePos = pow(s, 3) * gParticleSizeOverLifeTime.PointA
			+ 3 * pow(s, 2) * t * gParticleSizeOverLifeTime.PointB
			+ 3 * s * pow(t, 2) * gParticleSizeOverLifeTime.PointC
			+ pow(t, 3) * gParticleSizeOverLifeTime.PointD;
        
        sizeOut = sizePos.x;
    }
    else
    {
        sizeOut = sizeIn;
    }
}

void processRotationOverLifeTime(float rotateIn, out float rotateOut)
{
    if (gParticleRotationOverLifeTime.IsUsed)
    {
        rotateOut = rotateIn + gParticleRotationOverLifeTime.AngularVelocity * gParticleInstance.FrameTime * gParticleMain.SimulationSpeed;
    }
    else
    {
        rotateOut = rotateIn;
    }
}

float3 RGBtoHSV(float3 rgb)
{
    float cmax = max(rgb.r, max(rgb.g, rgb.b));
    float cmin = min(rgb.r, min(rgb.g, rgb.b));
    float delta = cmax - cmin;

    float3 hsv;
    hsv.z = cmax; // value
    if (cmax != 0)
        hsv.y = delta / cmax; // saturation
    else
    {
        hsv.y = 0;
        hsv.x = -1;
        return hsv;
    }

    if (rgb.r == cmax)
        hsv.x = (rgb.g - rgb.b) / delta; // between yellow & magenta
    else if (rgb.g == cmax)
        hsv.x = 2 + (rgb.b - rgb.r) / delta; // between cyan & yellow
    else
        hsv.x = 4 + (rgb.r - rgb.g) / delta; // between magenta & cyan

    hsv.x /= 6;
    if (hsv.x < 0)
        hsv.x += 1;
    return hsv;
}

float3 HSVtoRGB(float3 hsv)
{
    float h = hsv.x * 6;
    float s = hsv.y;
    float v = hsv.z;

    int i = floor(h);
    float f = h - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    float3 rgb;
    if (i == 0)
        rgb = float3(v, t, p);
    else if (i == 1)
        rgb = float3(q, v, p);
    else if (i == 2)
        rgb = float3(p, v, t);
    else if (i == 3)
        rgb = float3(p, q, v);
    else if (i == 4)
        rgb = float3(t, p, v);
    else
        rgb = float3(v, p, q);

    return rgb;
}

float4 OverlayMode(float4 src, float4 dst)
{
    float4 result;
    result.r = (dst.r < 0.5) ? (2.0 * src.r * dst.r) : (1.0 - 2.0 * (1.0 - src.r) * (1.0 - dst.r));
    result.g = (dst.g < 0.5) ? (2.0 * src.g * dst.g) : (1.0 - 2.0 * (1.0 - src.g) * (1.0 - dst.g));
    result.b = (dst.b < 0.5) ? (2.0 * src.b * dst.b) : (1.0 - 2.0 * (1.0 - src.b) * (1.0 - dst.b));
    result.a = src.a * dst.a; // Assuming alpha is multiplied
    return result;
}

float4 ColorMode(float4 src, float4 dst)
{
    float3 srcHsv = RGBtoHSV(src.rgb);
    float3 dstHsv = RGBtoHSV(dst.rgb);
    float3 resultRgb = HSVtoRGB(float3(srcHsv.r, srcHsv.g, dstHsv.b));
    return float4(resultRgb, src.a);
}