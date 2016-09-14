#pragma once

#include "ofMain.h"
#include "ofxBvh.h"
#include "ofxUI.h"
#include "ObjModel.h"
#include "FastLSM.h"
#include "Testbed.h"
#include <gl/gl.h>
#include <gl/glu.h>
#include <GL/glut.h>
#include "shadowMapLight.h"
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#include <mmsystem.h>
#include <time.h>

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		void exit();

		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		void drawMain();
		ofEasyCam cam;
		ofCamera cam2;
		ofLight light;

		// Obj
		ObjModel obj;

		// BVH
		ofxBvh bvh;
		string bvhFileName;
		void RenderIndex(int i);

		// MUSIC
		ofSoundPlayer sound;

		// GUI
		ofxUICanvas *gui1;
		ofxUICanvas *gui2; 
		ofxUICanvas *gui3;
		ofxUICanvas *gui4;
		ofxUICanvas *gui5;
		void guiSetup1();
		void guiSetup2();
		void guiSetup3();
		void guiSetup4();
		void guiSetup5();

		void guiEvent(ofxUIEventArgs &e);
		ofxUIMovingGraph *mg;

		VERTEX3D ErrorToRGB(float err, float errMax, float errMin);

		// Skinning
		float SkinWeight(ofVec3f P0, ofVec3f P1, ofVec3f P2, int c);
		void SkinUpdate();
		void setWeight();
		void setManhattanWeight();

		// Shader
		ofVbo vbomesh;
		ofMesh mesh;
		void Shader();
		int readShaderSource(GLuint shader, const char *file);
		void ofApp::printShaderInfoLog(GLuint shader);
		void ofApp::printProgramInfoLog(GLuint program);
		//vector<float> buffer;

		// LSM
		Testbed LSM;
		void reset();

		// Picture
		 ofImage grabbedImage;

		// shadow
        void setupLights();
        void createRandomObjects();
        void drawObjects();
   
        ofEasyCam m_cam;
        ShadowMapLight m_shadowLight;
        ofShader m_shader;
    
        float   m_angle;    
        bool    m_bDrawDepth;
        bool    m_bDrawLight;
        bool    m_bPaused;

		// calculation time
		LARGE_INTEGER start, end;
		LARGE_INTEGER freq;
		double renderingTime;
		double bvhTimeValue;

		LARGE_INTEGER startAll, endAll;
		LARGE_INTEGER freqAll;
		double allTime;

		LARGE_INTEGER startUpdate, endUpdate;
		LARGE_INTEGER freqUpdate;
		double updateTime;;
		
};
