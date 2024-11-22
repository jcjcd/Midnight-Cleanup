// Global variable access via cb
cbuffer outlineSettings
{
    float3 mainOutlineColor;
    int outlineThickness;
    float3 secondOutlineColor;
    int numSelections;
    int msaaOn;
};

Texture2D<int> pickingRTV;
RWTexture2D<float4> finalRenderTarget;

cbuffer SelectionListBuffer
{
    int Selection_0;
    int Selection_1;
    int Selection_2;
    int Selection_3;
    int Selection_4;
    int Selection_5;
    int Selection_6;
    int Selection_7;
    int Selection_8;
    int Selection_9;
	int Selection_10;
	int Selection_11;
	int Selection_12;
	int Selection_13;
	int Selection_14;
	int Selection_15;
	int Selection_16;
	int Selection_17;
	int Selection_18;
	int Selection_19;
}

#define MAX_SELECTIONS 20

[numthreads(16, 16, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    int texelID = pickingRTV[dispatchThreadID.xy];

    if (texelID != -1)
    {
        for (int i = 0; i < MAX_SELECTIONS; i++)
        {
            if (i >= numSelections)
                break;

            uint selection = 0;
            switch (i)
            {
                case 0: selection = Selection_0; break;
                case 1: selection = Selection_1; break;
                case 2: selection = Selection_2; break;
                case 3: selection = Selection_3; break;
                case 4: selection = Selection_4; break;
                case 5: selection = Selection_5; break;
                case 6: selection = Selection_6; break;
                case 7: selection = Selection_7; break;
                case 8: selection = Selection_8; break;
                case 9: selection = Selection_9; break;
				case 10: selection = Selection_10; break;
				case 11: selection = Selection_11; break;
				case 12: selection = Selection_12; break;
				case 13: selection = Selection_13; break;
				case 14: selection = Selection_14; break;
				case 15: selection = Selection_15; break;
				case 16: selection = Selection_16; break;
				case 17: selection = Selection_17; break;
				case 18: selection = Selection_18; break;
				case 19: selection = Selection_19; break;
            }

            if (texelID == selection)
            {
                float3 outlineColor = (i == 0) ? mainOutlineColor : secondOutlineColor;
                for (int row = -outlineThickness; row < outlineThickness; row++)
                {
                    for (int column = -outlineThickness; column < outlineThickness; column++)
                    {
                        uint2 texel = uint2(row, column);
                        uint texelIDKernel = pickingRTV[texel + dispatchThreadID.xy];
                        if (texelIDKernel != texelID)
                        {
                            finalRenderTarget[dispatchThreadID.xy] = float4(outlineColor, 1.f);
                            return;
                        }
                    }
                }
                break;
            }
        }
    }
}