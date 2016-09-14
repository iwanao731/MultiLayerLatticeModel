#pragma once
#include "Body.h"
#include <vector>
#include "ObjModel.h"
#include <windows.h>

class Testbed
{
public:
	static std::vector<Body*> bodies;

	static void Initialize(float f_stiff, float f_damp, float s_stiff, float s_damp, float m_stiff, float m_damp, float boneDist, float _fatRatio, int width, ofxBvh *bvh, ObjModel *src, ObjModel *surface, float _alpha);
	static void Update(ofxBvh *_bvh, ofxBvh *_origin, ObjModel *src, float fat, int count, bool distance, bool shear, bool collision, int vPreservingSwitch, float dist_alpha, float shear_alpha, ofVec3f ballPos, float radius);
	static void Render(int boneNum);
	static void Render2(int boneNum, bool _alpha);
	static void Render3(string _layerName);
	static void Render4(float fatRatio);
	static void Render5(string _layerName);
	static void SetLayer(float fatRatio, int boneIndex, float m_stiff, float m_damp, float f_stiff, float f_damp);

	static void RenderForceDirection(ofxBvh *bvh, bool condition, bool renderExternalForce);
	static void RenderSkin(ObjModel *surface, string colorName);
	static void RenderParticle(int boneNum, bool disp);
	static void Reset();
	static VERTEX3D ErrorToRGB(float err, float errMin, float errMax);
	static void Exit();

	// Volume Preserving
	static float SendInitialVolume();
	static float SendCurrentVolume();
	static float CalcVolume(ObjModel *surface);

	//nearestPick
	ofVec3f PointPick(int index, int mouse_x, int mouse_y, ofEasyCam cam);
	int getIndexPickingPoint(int mouse_x, int mouse_y, ofEasyCam cam);
	ofVec3f getVertex(int index);


protected:
	static int frame;
	static void AddBody(float f_stiff, float f_damp, float s_stiff, float s_damp, float m_stiff, float m_damp, float boneDist, float fatRatio, int width, ofxBvh *bvh, ObjModel *src, ObjModel *surface, float _alpha);
};
