#include <d3dx9.h>

#include "Gut.h"
#include "GutWin32.h"
#include "GutTexture_DX9.h"

#include "render_data.h"

static LPDIRECT3DTEXTURE9 g_pTexture = NULL;

bool InitResourceDX9(void)
{
	// ���oDirect3D 9�˸m
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();
	// �]�w�����ഫ�x�}
	int w, h;
	GutGetWindowSize(w, h);
	float aspect = (float) h / (float) w;
	Matrix4x4 projection_matrix = GutMatrixPerspectiveRH_DirectX(g_fFovW, aspect, 0.1f, 100.0f);
	device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX *) &projection_matrix);
	// ��������
	device->SetRenderState(D3DRS_LIGHTING, FALSE);
	// �e�X���V��ϦV���T����
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	// ���J�K��
	g_pTexture = GutLoadTexture_DX9("../../textures/brick.tga");

	return true;
}

bool ReleaseResourceDX9(void)
{
	SAFE_RELEASE(g_pTexture);

	return true;
}

void ResizeWindowDX9(int width, int height)
{
	// ���oDirect3D 9�˸m
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();
	// Reset Device
	GutResetGraphicsDeviceDX9();
	// �]�w�����ഫ�x�}
	int w, h;
	GutGetWindowSize(w, h);
	float aspect = (float) h / (float) w;
	Matrix4x4 projection_matrix = GutMatrixPerspectiveRH_DirectX(g_fFovW, aspect, 0.1f, 100.0f);
	device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX *) &projection_matrix);
	// ���Y�y�Шt���ഫ�x�}
	Matrix4x4 view_matrix = GutMatrixLookAtRH(g_eye, g_lookat, g_up);
	device->SetTransform(D3DTS_VIEW, (D3DMATRIX *) &view_matrix);
	// ��������
	device->SetRenderState(D3DRS_LIGHTING, FALSE);
	// �e�X���V��ϦV���T����
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
}

// �ϥ�DirectX 9��ø��
void RenderFrameDX9(void)
{
	LPDIRECT3DDEVICE9 device = GutGetGraphicsDeviceDX9();
	// �����e��
	device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	// �}�l�Uø�ϫ��O
	device->BeginScene(); 
	// ���Y�y�Шt���ഫ�x�}
	Matrix4x4 view_matrix = GutMatrixLookAtRH(g_eye, g_lookat, g_up);
	device->SetTransform(D3DTS_VIEW, (D3DMATRIX *) &view_matrix);
	// world space transfrom
	device->SetTransform(D3DTS_WORLD, (D3DMATRIX *) &g_world_matrix);
	// �]�w��Ʈ榡
	device->SetFVF(D3DFVF_XYZ); 
	// �M�ζK��
	device->SetTexture(0, g_pTexture);
	// �K�Ϯy�ЭȶW�L1�ɷ|�۰��ഫ��0-1�����Ȩӭ��ƶK��
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	// trilinear filter
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	// �ϥΦ۰ʲ��ͶK�Ϯy�Х\��
	device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
	// �}�ҶK�Ϯy�Яx�}�ഫ�\��
	device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	Matrix4x4 world_view_matrix = g_world_matrix * view_matrix;
	// ��K�Ϯy���ഫ�x�}�]��world*view���ϯx�}
	Matrix4x4 texture_matrix = world_view_matrix;
	texture_matrix.FastInvert();
	device->SetTransform(D3DTS_TEXTURE0, (D3DMATRIX *) &texture_matrix);

	// �e�X�x��
	device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, g_Quad, sizeof(Vertex_V));
	// �ŧi�Ҧ���ø�ϫ��O���U���F
	device->EndScene(); 
	// ��I��backbuffer���e���e�{�X��
    device->Present( NULL, NULL, NULL, NULL );
}