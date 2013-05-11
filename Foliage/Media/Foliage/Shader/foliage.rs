[RasterizerStateDX11]
CullMode = D3D11_CULL_NONE

[RenderTargetBlendStateDX11_1]
BlendEnable = true
SrcBlend  = D3D11_BLEND_ONE
DestBlend = D3D11_BLEND_INV_SRC_ALPHA
BlendOp   = D3D11_BLEND_OP_ADD

[DepthStencilStateDX11]
DepthEnable = true
DepthFunc = D3D11_COMPARISON_GREATER_EQUAL
DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO

[SamplerDX11_1]
MipLODBias = -1.0