#include <windows.h>

#include "glew.h" // OpenGL extension
// Standard OpenGL header
#include <GL/gl.h>
#include <GL/glu.h>
#include "Gut.h"
#include "GutWin32.h"
#include "GutModel_OpenGL.h"
#include "render_data.h"

// static Matrix4x4 g_view_matrix;
static Matrix4x4 g_projection_matrix;
static Matrix4x4 g_mirror_view_matrix;

static CGutModel_OpenGL g_Model_OpenGL;

static GLuint g_texture = 0;
static GLuint g_depthtexture = 0;
static GLuint g_framebuffer = 0;
static GLuint g_depthbuffer = 0;

bool InitResourceOpenGL(void)
{
	if ( glGenFramebuffersEXT==NULL )
	{
		printf("Could not create frame buffer object\n");
		return false;
	}

	// 投影矩陣
	g_projection_matrix = GutMatrixPerspectiveRH_OpenGL(g_fFOV, 1.0f, 0.1f, 100.0f);
	// 設定視角轉換矩陣
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( (float *) &g_projection_matrix);
	glMatrixMode(GL_MODELVIEW);	

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glDisable(GL_CULL_FACE);

	// 開啟一個framebuffer object
	glGenFramebuffersEXT(1, &g_framebuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_framebuffer);

	// 配罝一塊貼圖空間給framebuffer object繪圖使用 
	{
		glGenTextures(1, &g_texture);
		glBindTexture(GL_TEXTURE_2D, g_texture);
		// 設定filter
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// 宣告貼圖大小及格式
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		// framebuffer的RGBA繪圖
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, g_texture, 0);
	}

	// 配置zbuffer給framebuffer object使用
	{
		glGenRenderbuffersEXT(1, &g_depthbuffer);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, g_depthbuffer);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, 512, 512);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, g_depthbuffer);
	}

	// 檢查framebuffer object有沒有配置成?
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if ( status!=GL_FRAMEBUFFER_COMPLETE_EXT )
	{
		return false;
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	CGutModel::SetTexturePath("../../textures/");

	g_Model_OpenGL.ConvertToOpenGLModel(&g_Model);

	return true;
}

bool ReleaseResourceOpenGL(void)
{
	if ( g_framebuffer )
	{
		glDeleteFramebuffersEXT(1, &g_framebuffer);
		g_framebuffer = 0;
	}

	if ( g_depthbuffer )
	{
		glDeleteRenderbuffersEXT(1, &g_depthbuffer);
		g_depthbuffer = 0;
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
	g_projection_matrix = GutMatrixPerspectiveRH_OpenGL(g_fFOV, aspect, 0.1f, 100.0f);
	// 設定視角轉換矩陣
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( (float *) &g_projection_matrix);
}

void DrawImage(GLuint texture, float x, float y, float w, float h, bool bInvertV = false)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glUseProgram(0);

	sModelMaterial_OpenGL mtl;
	mtl.m_Textures[0] = texture;
	mtl.Submit();

	float TopV = bInvertV ? 1.0f : 0.0f;
	float BottomV = bInvertV ? 0.0f : 1.0f;

	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, BottomV);
		glVertex3f(x, y, 0.0f); 

		glTexCoord2f(1.0f, BottomV);
		glVertex3f(x+w, y, 0.0f);
		
		glTexCoord2f(1.0f, TopV);
		glVertex3f(x+w, y+h, 0.0f);

		glTexCoord2f(0.0f, TopV);
		glVertex3f(x, y+h, 0.0f);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void SetupLightingOpenGL(void)
{
	int LightID = GL_LIGHT0;

	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);

	glEnable(LightID);
	
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &g_vAmbientLight[0]);
	glLightfv(LightID, GL_POSITION, &g_Light.m_Position[0]);
	glLightfv(LightID, GL_DIFFUSE, &g_Light.m_Diffuse[0]);
	glLightfv(LightID, GL_SPECULAR, &g_Light.m_Specular[0]);

	glLightf(LightID, GL_CONSTANT_ATTENUATION,	1.0f);
	glLightf(LightID, GL_LINEAR_ATTENUATION,	0.0f);
	glLightf(LightID, GL_QUADRATIC_ATTENUATION, 0.0f);
}

// 使用OpenGL來繪圖
void RenderFrameOpenGL(void)
{
	Matrix4x4 light_projection_matrix;
	Matrix4x4 light_view_matrix;
	Matrix4x4 light_world_view_matrix;

	Matrix4x4 view_matrix =  g_Control.GetViewMatrix();
	Matrix4x4 world_matrix = g_Control.GetObjectMatrix();
	
	{
		// `使用` g_framebuffer framebuffer object	
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_framebuffer);
		glViewport(0, 0, 512, 512);
		// `清除畫面`
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// `光源位置`
		Vector4 vLightPos = g_Light.m_Position;
		Vector4 vLightUp(0.0f, 1.0f, 0.0f);
		Vector4 vLightLookat(0.0f, 0.0f, 0.0f);
		// `設定光源的轉換矩陣`
		light_projection_matrix = GutMatrixPerspectiveRH_OpenGL(60.0f, 1.0f, 0.1f, 100.0f);
		light_view_matrix = GutMatrixLookAtRH(vLightPos, vLightLookat, vLightUp);
		light_world_view_matrix = world_matrix * light_view_matrix;

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf( (float *) &light_projection_matrix );
		
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf( (float *) &light_world_view_matrix );

		glDisable(GL_LIGHTING);

		// `畫出黑色的茶壼`
		sModelMaterial_OpenGL material;
		material.m_bCullFace = false;
		material.m_Diffuse.Set(0.0);
		material.Submit(NULL);

		g_Model_OpenGL.Render(0);
	}

	{
		// `使用主framebuffer object, 也就是視窗.`
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		int w, h;
		GutGetWindowSize(w, h);
		glViewport(0, 0, w, h);

		// `清除畫面`
		glClearColor(0.0f, 0.0f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// `計算出一個可以轉換到鏡頭座標系的矩陣`
		Matrix4x4 view_matrix = g_Control.GetViewMatrix();
		
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf( (float *) &view_matrix);
		SetupLightingOpenGL();

		Matrix4x4 world_view_matrix = world_matrix * view_matrix;

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf( (float *) &g_projection_matrix);

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf( (float *) &world_view_matrix);

		g_Model_OpenGL.Render();

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf( (float *) &view_matrix);

		glDisable(GL_LIGHTING);

		sModelMaterial_OpenGL material;
		material.m_Diffuse.Set(1.0f);
		material.m_Textures[0] = g_texture;
		material.Submit(NULL);

		Matrix4x4 uv_offset_matrix;

		uv_offset_matrix.Identity();
		uv_offset_matrix.Scale(0.5f, 0.5f, 0.5f);
		uv_offset_matrix[3].Set(0.5f, 0.5f, 0.5f, 1.0f);

		Matrix4x4 texture_matrix = light_view_matrix * light_projection_matrix * uv_offset_matrix;

		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf( (float *)&texture_matrix);

		glDisable(GL_LIGHTING);

		// `設定頂點資料格式`
		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

		glEnableClientState(GL_VERTEX_ARRAY);
		glClientActiveTexture(GL_TEXTURE0_ARB);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		//
		glVertexPointer(3, GL_FLOAT, sizeof(Vertex_VT), &g_Quad[0].m_Position);
		// `直接把頂點資料拿來當貼圖座標使用`
		glTexCoordPointer(4, GL_FLOAT, sizeof(Vertex_VT), &g_Quad[0].m_Position);
		// `畫出看板`
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glPopClientAttrib();

		glLoadIdentity();
	}

	// 把背景backbuffer的畫面呈現出來
	GutSwapBuffersOpenGL();
}
