#include "Gut.h"
#include "GutDX9.h"
#include "GutWin32.h"
#include "GutTexture_DX9.h"

#include "render_data.h"

LPDIRECT3DTEXTURE9 g_pTexture = NULL;
LPDIRECT3DVERTEXSHADER9 g_pVertexShader = NULL;
LPDIRECT3DPIXELSHADER9 g_pPixelShader = NULL;

LPDIRECT3DTEXTURE9 g_pFrameBuffer[2] = {NULL, NULL};
LPDIRECT3DSURFACE9 g_pFrameSurface[2] = {NULL, NULL};

static sImageInfo g_ImageInfo;

bool ReInitResourceDX9(void)
{
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();

	device->SetSamplerState(0,  D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0,  D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0,  D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	device->SetSamplerState(0,  D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0,  D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	device->SetRenderState(D3DRS_LIGHTING, FALSE);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	return true;
}

bool InitResourceDX9(void)
{
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();

	if ( !ReInitResourceDX9() )
		return false;
	
	g_pTexture = GutLoadTexture_DX9("../../textures/lena.tga", &g_ImageInfo);
	if ( NULL==g_pTexture )
		return false;
	
	g_pVertexShader = GutLoadVertexShaderDX9_HLSL("../../shaders/Posteffect_blur.hlsl", "VS", "vs_2_0");
	g_pPixelShader = GutLoadPixelShaderDX9_HLSL("../../shaders/Posteffect_blur.hlsl", "PS", "ps_2_0");
	if ( NULL==g_pVertexShader || NULL==g_pPixelShader )
		return false;

	device->CreateTexture(g_ImageInfo.m_iWidth, g_ImageInfo.m_iHeight, 1, 
		D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, 
		D3DPOOL_DEFAULT, &g_pFrameBuffer[0], NULL);
	
	device->CreateTexture(g_ImageInfo.m_iWidth, g_ImageInfo.m_iHeight, 1, 
		D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, 
		D3DPOOL_DEFAULT, &g_pFrameBuffer[1], NULL);

	if ( NULL==g_pFrameBuffer[0] || NULL==g_pFrameBuffer[1] )
		return false;

	g_pFrameBuffer[0]->GetSurfaceLevel(0, &g_pFrameSurface[0]);
	g_pFrameBuffer[1]->GetSurfaceLevel(0, &g_pFrameSurface[1]);

	return true;
}

bool ReleaseResourceDX9(void)
{
	SAFE_RELEASE(g_pTexture);
	SAFE_RELEASE(g_pVertexShader);
	SAFE_RELEASE(g_pPixelShader);

	g_pFrameSurface[0]->Release();
	g_pFrameSurface[1]->Release();

	SAFE_RELEASE(g_pFrameBuffer[0]);
	SAFE_RELEASE(g_pFrameBuffer[1]);

	return true;
}

void ResizeWindowDX9(int width, int height)
{
	// Reset Device
	GutResetGraphicsDeviceDX9();
	// 取得Direct3D 9裝置
	ReInitResourceDX9();
}

LPDIRECT3DTEXTURE9 BlurImageDX9(LPDIRECT3DTEXTURE9 pSource, sImageInfo *pInfo)
{
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();

	// 計算貼圖像素的貼圖座標間距
	float fTexelW = 1.0f/(float)pInfo->m_iWidth;
	float fTexelH = 1.0f/(float)pInfo->m_iHeight;

	const int num_samples = KERNELSIZE;
	Vector4 vTexOffsetX[num_samples];
	Vector4 vTexOffsetY[num_samples];
	// 計算出隣近像素的貼圖座標間距
	for ( int i=0; i<num_samples; i++ )
	{
		vTexOffsetX[i].Set( g_uv_offset_table[i] * fTexelW, 0.0f, 0.0f, g_weight_table[i]);
		vTexOffsetY[i].Set(0.0f, g_uv_offset_table[i] * fTexelH, 0.0f, g_weight_table[i]);
	}

	Matrix4x4 transform_mat; transform_mat.Identity();
	// Direct3D9會偏移1個像素, 帶入一個偏移值把它修正回來.
	Vector4 vTexoffset(fTexelW*0.5f, fTexelH*0.5f, 0.0f);

	LPDIRECT3DSURFACE9 pRenderTargetBackup, pDepthStencilBackup;
	device->GetRenderTarget(0, &pRenderTargetBackup);
	device->GetDepthStencilSurface(&pDepthStencilBackup);

	// 使用模糊化Shader
	device->SetPixelShader(g_pPixelShader);
	device->SetVertexShader(g_pVertexShader);

	device->SetVertexShaderConstantF(0, (float *)&transform_mat, 4);
	device->SetVertexShaderConstantF(4, (float *)&vTexoffset, 1);
	// 設定頂點資料格式	
	device->SetFVF(D3DFVF_XYZ|D3DFVF_TEX1);
	// 套用貼圖
	device->SetTexture(0, g_pTexture);

	device->SetDepthStencilSurface(NULL);
	
	// 水平方向模糊
	{
		device->SetRenderTarget(0, g_pFrameSurface[0]);
		device->SetTexture(0, pSource);
		device->SetPixelShaderConstantF(0, (float *)vTexOffsetX, num_samples);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, g_FullScreenQuad, sizeof(Vertex_VT));
	}
	// 垂直方向模糊
	{
		device->SetRenderTarget(0, g_pFrameSurface[1]);
		device->SetTexture(0, g_pFrameBuffer[0]);
		device->SetPixelShaderConstantF(0, (float *)vTexOffsetY, num_samples);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, g_FullScreenQuad, sizeof(Vertex_VT));
	}

	device->SetRenderTarget(0, pRenderTargetBackup);
	device->SetDepthStencilSurface(pDepthStencilBackup);
	
	SAFE_RELEASE(pRenderTargetBackup);
	SAFE_RELEASE(pDepthStencilBackup);

	return g_pFrameBuffer[1];
}

// 使用DirectX 9來繪圖
void RenderFrameDX9(void)
{
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();

	device->BeginScene(); 
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
	Matrix4x4 ident_mat; ident_mat.Identity();

	LPDIRECT3DTEXTURE9 pResult = g_pTexture;

	if ( g_bPosteffect )
	{
		// 模糊化處理
		pResult = BlurImageDX9(g_pTexture, &g_ImageInfo);
	}

	// 用Fixed Pipeline來畫出貼圖內容
	device->SetPixelShader(NULL);
	device->SetVertexShader(NULL);
	device->SetTexture(0, pResult);
	device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, g_FullScreenQuad, sizeof(Vertex_VT));

	device->EndScene();
	
	// 把背景backbuffer的畫面呈現出來
    device->Present( NULL, NULL, NULL, NULL );
}
