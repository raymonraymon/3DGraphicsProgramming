#include <windows.h>

#include "glew.h" // OpenGL extension
#include <GL/gl.h>

#include "Gut.h"
#include "GutTexture_OpenGL.h"
#include "GutModel_OpenGL.h"

#include "render_data.h"

static Matrix4x4 g_view_matrix;
GLuint g_TextureID = 0;
CGutModel_OpenGL g_Model_OpenGL;

bool InitResourceOpenGL(void)
{
	// 鏡頭座標系轉換矩陣
	g_view_matrix = GutMatrixLookAtRH(g_eye, g_lookat, g_up);
	// 設定投影矩陣
	Matrix4x4 projection_matrix = GutMatrixPerspectiveRH_OpenGL(g_fFovW, 1.0f, 0.1f, 100.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( (float *) &projection_matrix);
	// 載入貼圖
	const char *texture_array[] = {
		"../../textures/uffizi_right.tga",
		"../../textures/uffizi_left.tga",
		"../../textures/uffizi_top.tga",
		"../../textures/uffizi_bottom.tga",
		"../../textures/uffizi_back.tga", // `右手座標系上 Z+ 為鏡頭後方.`
		"../../textures/uffizi_front.tga" // `右手座標系上 Z- 為鏡頭前方.`
	};

	g_TextureID = GutLoadCubemapTexture_OpenGL(texture_array);

	// 使用CUBEMAP貼圖功能
	glEnable(GL_TEXTURE_CUBE_MAP);
	// 套用CUBEMAP貼圖
	glBindTexture( GL_TEXTURE_CUBE_MAP, g_TextureID );

	g_Model_OpenGL.ConvertToOpenGLModel(&g_Model);

	return true;
}

bool ReleaseResourceOpenGL(void)
{
	if ( g_TextureID )
	{
		glDeleteTextures(1, &g_TextureID);
		g_TextureID = 0;
	}
	return true;
}

// callback function. 視窗大小改變時會被呼叫, 並傳入新的視窗大小.
void ResizeWindowOpenGL(int width, int height)
{
	// 使用新的視窗大小做為新的繪圖解析度
	glViewport(0, 0, width, height);
	// 投影矩陣, 重設水平跟垂直方向的視角.
	float aspect = (float) height / (float) width;
	Matrix4x4 projection_matrix = GutMatrixPerspectiveRH_OpenGL(g_fFovW, aspect, 0.1f, 100.0f);
	// 設定視角轉換矩陣
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( (float *) &projection_matrix);
}

// 使用OpenGL來繪圖
void RenderFrameOpenGL(void)
{
	// 清除畫面
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// 把正向跟反向的面都畫出來
	glDisable(GL_CULL_FACE);
	// 使用CUBEMAP貼圖功能
	glEnable(GL_TEXTURE_CUBE_MAP);
	// 套用CUBEMAP貼圖
	glBindTexture( GL_TEXTURE_CUBE_MAP, g_TextureID );
	// 開啟自動產生貼圖座標功能
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	// 以反射的值為貼圖座標
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
	// Trilinear filter
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	// 使用自動normalize功能
	glEnable(GL_NORMALIZE);
	// 計算出一個可以轉換到鏡頭座標系的矩陣
	Matrix4x4 view_matrix = GutMatrixLookAtRH(g_eye, g_lookat, g_up);
	Matrix4x4 world_view_matrix = g_world_matrix * view_matrix;
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf( (float *) &world_view_matrix);
	// 傳入0代表不套用模型中的材質, 經由外部來設定.
	g_Model_OpenGL.Render(0);
	// 把背景backbuffer的畫面呈現出來
	GutSwapBuffersOpenGL();
}
