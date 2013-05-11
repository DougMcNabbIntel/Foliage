
[RasterizerStateDX11] 
CullMode = D3D11_CULL_NONE 

[RenderTargetBlendStateDX11_1]
BlendEnable    = true
SrcBlend       = D3D11_BLEND_ONE
DestBlend      = D3D11_BLEND_ZERO
BlendOp        = D3D11_BLEND_OP_ADD
SrcBlendAlpha  = D3D11_BLEND_SRC_ALPHA
DestBlendAlpha = D3D11_BLEND_ONE
BlendOpAlpha   = D3D11_BLEND_OP_MAX

[DepthStencilStateDX11]
DepthEnable     = true

[SamplerDX11_1]
Filter = D3D11_FILTER_ANISOTROPIC
MaxAnisotropy = 8
MipLODBias = -1.0

