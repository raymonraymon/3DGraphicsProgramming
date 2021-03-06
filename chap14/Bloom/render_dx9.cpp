#include "Gut.h"
#include "GutDX9.h"
#include "GutWin32.h"
#include "GutTexture_DX9.h"

#include "render_data.h"

LPDIRECT3DTEXTURE9 g_pTexture = NULL;
LPDIRECT3DVERTEXSHADER9 g_pVertexShader = NULL;
LPDIRECT3DPIXELSHADER9 g_pBlurPS = NULL;
LPDIRECT3DPIXELSHADER9 g_pBrightnessPS = NULL;
LPDIRECT3DPIXELSHADER9 g_pAddPS = NULL;

LPDIRECT3DTEXTURE9 g_pFrameBuffer[3] = {NULL, NULL};
LPDIRECT3DSURFACE9 g_pFrameSurface[3] = {NULL, NULL};

static sImageInfo g_ImageInfo;
static Matrix4x4 g_wvp_matrix;

bool ReInitResourceDX9(void)
{
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();
	
	int width = g_ImageInfo.m_iWidth/4;
	int height = g_ImageInfo.m_iHeight/4;

	device->CreateTexture(width, height, 1, 
		D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, 
		D3DPOOL_DEFAULT, &g_pFrameBuffer[0], NULL);
	
	device->CreateTexture(width, height, 1, 
		D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, 
		D3DPOOL_DEFAULT, &g_pFrameBuffer[1], NULL);

	device->CreateTexture(width, height, 1, 
		D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, 
		D3DPOOL_DEFAULT, &g_pFrameBuffer[2], NULL);

	if ( NULL==g_pFrameBuffer[0] || NULL==g_pFrameBuffer[1] || NULL==g_pFrameBuffer[2] )
		return false;

	g_pFrameBuffer[0]->GetSurfaceLevel(0, &g_pFrameSurface[0]);
	g_pFrameBuffer[1]->GetSurfaceLevel(0, &g_pFrameSurface[1]);
	g_pFrameBuffer[2]->GetSurfaceLevel(0, &g_pFrameSurface[2]);

	for ( int i=0; i<4; i++ )
	{
		device->SetSamplerState(i,  D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i,  D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i,  D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		device->SetSamplerState(i,  D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(i,  D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	device->SetRenderState(D3DRS_LIGHTING, FALSE);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	Matrix4x4 ident_mat; ident_mat.Identity();

	device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX *)&ident_mat);
	device->SetTransform(D3DTS_VIEW, (D3DMATRIX *)&ident_mat);
	device->SetTransform(D3DTS_WORLD, (D3DMATRIX *)&ident_mat);

	return true;
}

bool InitResourceDX9(void)
{
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();

	g_pTexture = GutLoadTexture_DX9("../../textures/space.tga", &g_ImageInfo);
	if ( NULL==g_pTexture )
		return false;
	
	g_pVertexShader = GutLoadVertexShaderDX9_HLSL("../../shaders/Posteffect_blur.hlsl", "VS", "vs_2_0");
	g_pBlurPS = GutLoadPixelShaderDX9_HLSL("../../shaders/Posteffect_blur.hlsl", "PS", "ps_2_0");
	g_pBrightnessPS = GutLoadPixelShaderDX9_HLSL("../../shaders/Posteffect_Brightness.hlsl", "PS", "ps_2_0");
	g_pAddPS = GutLoadPixelShaderDX9_HLSL("../../shaders/Posteffect_Add.hlsl", "PS", "ps_2_0");
	
	if ( NULL==g_pVertexShader || NULL==g_pBlurPS || 
		 NULL==g_pBrightnessPS || NULL==g_pAddPS )
		return false;

	if ( !ReInitResourceDX9() )
		return false;

	return true;
}

bool ReleaseResourceDX9(void)
{
	SAFE_RELEASE(g_pTexture);
	SAFE_RELEASE(g_pVertexShader);
	SAFE_RELEASE(g_pBlurPS);
	SAFE_RELEASE(g_pBrightnessPS);
	SAFE_RELEASE(g_pAddPS);

	for ( int i=0; i<3; i++ )
	{
		SAFE_RELEASE(g_pFrameSurface[i]);
		SAFE_RELEASE(g_pFrameBuffer[i]);
	}

	return true;
}

void ResizeWindowDX9(int width, int height)
{
	for ( int i=0; i<3; i++ )
	{
		SAFE_RELEASE(g_pFrameSurface[i]);
		SAFE_RELEASE(g_pFrameBuffer[i]);
	}

	// Reset Device
	GutResetGraphicsDeviceDX9();
	// 取得Direct3D 9裝置
	ReInitResourceDX9();
}

// `降低亮度, 同時縮小圖片.`
LPDIRECT3DTEXTURE9 Brightness(LPDIRECT3DTEXTURE9 pSource, sImageInfo *pInfo)
{
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();

	// `計算貼圖像素的貼圖座標間距`
	int w = pInfo->m_iWidth/4;
	int h = pInfo->m_iHeight/4;

	float fTexelW = 1.0f/(float)w;
	float fTexelH = 1.0f/(float)h;

	Matrix4x4 ident_mat; ident_mat.Identity();
	// `Direct3D9會偏移半個像素, 從貼圖座標來把它修正回來.`
	Vector4 vTexoffset(fTexelW*0.5f, fTexelH*0.5f, 0.0f, 1.0f);

	LPDIRECT3DSURFACE9 pRenderTargetBackup, pDepthStencilBackup;
	device->GetRenderTarget(0, &pRenderTargetBackup);
	device->GetDepthStencilSurface(&pDepthStencilBackup);

	// `使用改變亮度的Shader`
	device->SetPixelShader(g_pBrightnessPS);
	device->SetVertexShader(g_pVertexShader);

	device->SetVertexShaderConstantF(0, (float *)&ident_mat, 4);
	device->SetVertexShaderConstantF(4, (float *)&vTexoffset, 1);

	device->SetPixelShaderConstantF(0, (float *)&g_vBrightnessOffset, 1);
	device->SetPixelShaderConstantF(1, (float *)&g_vBrightnessScale, 1);

	// `設定頂點資料格式`
	device->SetFVF(D3DFVF_XYZ|D3DFVF_TEX1);

	device->SetRenderTarget(0, g_pFrameSurface[2]);
	device->SetDepthStencilSurface(NULL);

	device->SetTexture(0, pSource);
	device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, g_FullScreenQuad, sizeof(Vertex_VT));

	device->SetRenderTarget(0, pRenderTargetBackup);
	device->SetDepthStencilSurface(pDepthStencilBackup);
	
	SAFE_RELEASE(pRenderTargetBackup);
	SAFE_RELEASE(pDepthStencilBackup);

	return g_pFrameBuffer[2];
}

LPDIRECT3DTEXTURE9 BlurImageDX9(LPDIRECT3DTEXTURE9 pSource, sImageInfo *pInfo)
{
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();

	// `計算貼圖像素的貼圖座標間距`
	int w = pInfo->m_iWidth/4;
	int h = pInfo->m_iHeight/4;

	float fTexelW = 1.0f/(float)w;
	float fTexelH = 1.0f/(float)h;

	// `Direct3D9會偏移半個像素, 從貼圖座標來把它修正回來.`
	Vector4 vTexoffset(fTexelW*0.5f, fTexelH*0.5f, 0.0f, 1.0f);

	const int num_samples = KERNELSIZE;
	Vector4 vTexOffsetX[num_samples];
	Vector4 vTexOffsetY[num_samples];
	// `計算出隣近像素的貼圖座標間距`
	for ( int i=0; i<num_samples; i++ )
	{
		vTexOffsetX[i].Set(g_uv_offset_table[i] * fTexelW, 0.0f, 0.0f, g_weight_table[i]);
		vTexOffsetY[i].Set(0.0f, g_uv_offset_table[i] * fTexelH, 0.0f, g_weight_table[i]);
	}

	Matrix4x4 ident_mat; ident_mat.Identity();

	LPDIRECT3DSURFACE9 pRenderTargetBackup, pDepthStencilBackup;
	device->GetRenderTarget(0, &pRenderTargetBackup);
	device->GetDepthStencilSurface(&pDepthStencilBackup);

	// `使用模糊化Shader`
	device->SetPixelShader(g_pBlurPS);
	device->SetVertexShader(g_pVertexShader);

	device->SetVertexShaderConstantF(0, (float *)&ident_mat, 4);
	device->SetVertexShaderConstantF(4, (float *)&vTexoffset, 1);
	// `設定頂點資料格式`
	device->SetFVF(D3DFVF_XYZ|D3DFVF_TEX1);
	
	// `水平方向模糊`
	{
		device->SetRenderTarget(0, g_pFrameSurface[0]);
		device->SetDepthStencilSurface(NULL);
		device->SetTexture(0, pSource);
		device->SetPixelShaderConstantF(0, (float *)vTexOffsetX, num_samples);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, g_FullScreenQuad, sizeof(Vertex_VT));
	}
	// `垂直方向模糊`
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

// `使用Direct3D9來繪圖`
void RenderFrameDX9(void)
{
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();
	device->BeginScene(); 

	// `用Fixed Pipeline畫出原始貼圖內容`
	device->SetPixelShader(NULL);
	device->SetVertexShader(NULL);
	device->SetTexture(0, g_pTexture);
	device->SetTexture(1, NULL);
	device->SetFVF(D3DFVF_XYZ|D3DFVF_TEX1);
	device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, g_FullScreenQuad, sizeof(Vertex_VT));

	if ( g_bPosteffect )
	{
		LPDIRECT3DTEXTURE9 pResult = NULL;
		// `改變亮度`
		pResult = Brightness(g_pTexture, &g_ImageInfo);
		// `模糊化處理`
		pResult = BlurImageDX9(pResult, &g_ImageInfo);
		// `把影像處理的結果跟原始圖片相加`
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

		device->SetTexture(0, pResult);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, g_FullScreenQuad, sizeof(Vertex_VT));

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	}

	device->EndScene();
    device->Present( NULL, NULL, NULL, NULL );
}
