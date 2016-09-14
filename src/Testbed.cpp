#include <iostream>
#include "Testbed.h"
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <GL/glut.h>
#include <ctime>

std::vector<Body*> Testbed::bodies;
int Testbed::frame = 0;

// time
double skinningTime;
double distanceContraintTime;
double volumeCopnstraintTime;
double shapeMatchingTime;
double velocityPredictTime;
double velocityUpdateTime;
int frameNum;

//--------------------------------------------------------------
void Testbed::Initialize(float f_stiff, float f_damp, float s_stiff, float s_damp, float m_stiff, float m_damp, float boneDist, float _fatRatio, int width, ofxBvh *bvh, ObjModel *src, ObjModel *surface, float _alpha)
{
	srand((unsigned int)time(NULL));
	AddBody(f_stiff, f_damp, s_stiff, s_damp, m_stiff, m_damp, boneDist, _fatRatio, width, bvh, src, surface, _alpha);

	// time
	skinningTime = 0.0;
	distanceContraintTime = 0.0;
	volumeCopnstraintTime = 0.0;
	shapeMatchingTime = 0.0;
	velocityPredictTime = 0.0;
	velocityUpdateTime = 0.0;

	frameNum = 0;
}

//--------------------------------------------------------------
void Testbed::Reset()
{
	Testbed::bodies.clear();
	Testbed::bodies.empty();
}

//--------------------------------------------------------------
void Testbed::AddBody(float f_stiff, float f_damp, float s_stiff, float s_damp, float m_stiff, float m_damp, float boneDist, float fatRatio, int width, ofxBvh *bvh, ObjModel *src, ObjModel *surface, float _alpha)
{
	Body *body = new Body(Vector3f(10.0f,10.0f,10.0f));
//	Body *body = new Body(Vector3f(1.0f,1.0f,1.0f));
	body->w = width; //rand() % 5 + 1;
	body->alpha = f_stiff;	//0.75f
	body->fracturing = true;
	body->fractureDistanceTolerance = 999.0f;
	body->fractureRotationTolerance = 0.6f;	//0.6;
	body->kRegionDamping = f_damp; //0.25f;
	bodies.push_back(body);

	float scale = 1;
	for(int i=0; i<src->GetVertexNum(); i++){
		Point3 v(src->GetVertex(i).x*scale, src->GetVertex(i).y*scale, src->GetVertex(i).z*scale);
		body->AddParticle(v);
	}

	body->Finalize(surface);
	body->source = body->particles;	//original shape

	// Multi-Layer Latticeモデル構築
	body->SetupMultiLayerModel(bvh, boneDist, fatRatio, f_stiff, f_damp, s_stiff, s_damp, m_stiff, m_damp);

	// Calculate Skinning Weight
	//body->CalcEuclidWeight(bvh);
	//body->CalcManhattanWeight(bvh);
	body->CalcDionneWeight(bvh, _alpha);
	body->Skinning(bvh, bvh, src, fatRatio);
	body->ShapeMatch();

	// Volume Preserving Pre computing Term
	body->initVolumeConstraint();
	//body->SetUpVolumePreserving(bvh);
	//body->setUpMeshVolumePreserving(surface);
}

//--------------------------------------------------------------
void Testbed::Update(ofxBvh *_bvh, ofxBvh *_origin, ObjModel *src, float fat, int count, bool distance, bool shear, bool collision, int vPreservingSwitch, float dist_alpha, float shear_alpha, ofVec3f ballPos, float radius )
{
	const int MAX_BODIES = 500;
	const int SPAWN_RATE = 20;
	const int TARGET_FPS = 20;
	const float h = 1.0f / TARGET_FPS;

	if(frame % SPAWN_RATE == 0 && bodies.size() < MAX_BODIES)
		frame++;

	frameNum++;
	
	//int counter = 0;
	for each(Body *body in bodies)
	{
		LARGE_INTEGER start, end;
		LARGE_INTEGER freq;

		//-------------------------------------------------------------
		if( !QueryPerformanceFrequency(&freq) ){}
		if( !QueryPerformanceCounter(&start) ){ break; }

		body->Skinning(_bvh, _origin, src, fat);

		if( !QueryPerformanceCounter(&end) ){ break; }
		skinningTime += (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000;
		//-------------------------------------------------------------
	
		//-------------------------------------------------------------
		if( !QueryPerformanceFrequency(&freq) ){}
		if( !QueryPerformanceCounter(&start) ){ break; }

		body->ApplyParticleVelocities(h);

		if( !QueryPerformanceCounter(&end) ){ break; }
		velocityPredictTime += (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000;
		//-------------------------------------------------------------

		//-------------------------------------------------------------
		if( !QueryPerformanceFrequency(&freq) ){}
		if( !QueryPerformanceCounter(&start) ){ break; }

		if(distance)	body->UpdateDistanceConstraint(dist_alpha);
		if(shear)		body->UpdateShearConstraint(shear_alpha);
		if(collision)	body->SelfCollisionDetection(5.0);

		if( !QueryPerformanceCounter(&end) ){ break; }
		distanceContraintTime += (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000;
		//-------------------------------------------------------------

		//-------------------------------------------------------------
		if( !QueryPerformanceFrequency(&freq) ){}
		if( !QueryPerformanceCounter(&start) ){ break; }

		body->computeVolume();
		if(vPreservingSwitch == 1) body->updateVolumeConstraint();

		if( !QueryPerformanceCounter(&end) ){ break; }
		volumeCopnstraintTime += (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000;
		//-------------------------------------------------------------

		//-------------------------------------------------------------
		if( !QueryPerformanceFrequency(&freq) ){}
		if( !QueryPerformanceCounter(&start) ){ break; }

		// Do FastLSM simulation
		body->ShapeMatch();		

		if( !QueryPerformanceCounter(&end) ){ break; }
		shapeMatchingTime += (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000;
		//-------------------------------------------------------------

		//-------------------------------------------------------------
		if( !QueryPerformanceFrequency(&freq) ){}
		if( !QueryPerformanceCounter(&start) ){ break; }

		body->CalculateParticleVelocities(h);
		body->PerformRegionDamping();

		if( !QueryPerformanceCounter(&end) ){ break; }
		velocityUpdateTime += (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000;
		//-------------------------------------------------------------
	}
}

//--------------------------------------------------------------
void Testbed::Exit()
{
	cout << fixed << "skinningTime:	" << skinningTime/frameNum << " [ms]" << endl;
	cout << "velocityPredictTime:	" << velocityPredictTime/frameNum << " [ms]" << endl;
	cout << "distanceContraintTime:	" << distanceContraintTime/frameNum << " [ms]" << endl;
	cout << "volumeCopnstraintTime:	" << volumeCopnstraintTime/frameNum << " [ms]" << endl;
	cout << "shapeMatchingTime:	" << shapeMatchingTime/frameNum << " [ms]" << endl;
	cout << "velocityUpdateTime:	" << velocityUpdateTime/frameNum << " [ms]" << endl;
}

//--------------------------------------------------------------
float Testbed::SendInitialVolume()
{
	float volume;
	for each(Body *body in bodies)
	{
		volume = body->InitialVolume;
	}
	return volume;
}

//--------------------------------------------------------------
float Testbed::SendCurrentVolume()
{
	float volume;
	for each(Body *body in bodies)
	{
		volume = body->currentVolume;
	}
	return volume;
}

//--------------------------------------------------------------
void glVertex(Vector3f v)
{
	glVertex3f(v.x, v.y, v.z);
}

//--------------------------------------------------------------
void glNormal(Vector3f v)
{
	glNormal3f(v.x, v.y, v.z);
}

//--------------------------------------------------------------
void Testbed::Render(int boneNum)
{
	// Position the camera
	//gluLookAt(0, 1, 3, 0, 0, 0, 0, 1, 0);

	// Draw the cells
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);

	glMaterialf(GL_FRONT, GL_SHININESS, 28);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	for each(Body *body in bodies)
	{
		for each(Particle *particle in body->particles)
		{
			glPushMatrix();
			VERTEX3D v(particle->x.x, particle->x.y, particle->x.z);
			glTranslatef(v.x, v.y, v.z);
			VERTEX3D error = ErrorToRGB(particle->w[boneNum], 0.0, 1.0);
			glColor4f(error.x, error.y, error.z, 1.0);
			//glColor4f(0.753f, 0.764f, 0.910f, 1.0);
			glutSolidCube(9.98);
			glEnable(GL_DEPTH_TEST);
			glColor4f(0.0f, 0.0f, 0.0f, 1.0);
			glutWireCube(10);
			glPopMatrix();
		}
	}
	glDisable(GL_LIGHTING);
	glDisable(GL_NORMALIZE);
}

//--------------------------------------------------------------
void Testbed::Render2(int boneNum, bool _alpha)
{
	// Position the camera
	//gluLookAt(-1, 0, 0, 0, 0, 0, 0, 1, 0);

	// Draw the floor
	//glDisable(GL_LIGHTING);
	//glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
	//glBegin(GL_QUADS);
	//glVertex3f(-9999, -140.0, -9999);
	//glVertex3f(-9999, -140.0, 9999);
	//glVertex3f(9999, -140.0, 9999);
	//glVertex3f(9999, -140.0, -9999);
	//glEnd();
	
	// Draw the cells
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);

	glMaterialf(GL_FRONT, GL_SHININESS, 28);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);

	//glColor4f(0.753f, 0.764f, 0.910f, 1.0);

	// Draw the quads of the cells
	glBegin(GL_QUADS);
	for each(Body *body in bodies)
	{
		int i=0;
		body->UpdateCellPositions();

		for each(Cell *c in body->cells)
		{
			float col;
			if(boneNum>=0){
				float dist = body->latticeLocations.at(i)->particle->w.at(boneNum);
				col = ofMap(dist, 0, 1, 0, 255);
			}else{
				col = 0;
			}
			
			VERTEX3D error = ErrorToRGB(col, 0.0, 255);
			if(_alpha)
				ofSetColor(error.x, error.y, error.z, 255);
			else
				ofSetColor(error.x, error.y, error.z, 0);

//			ofSetColor( 255, 180-col/2, 0, _alpha);
			//std::cout << body->latticeLocations.at(i)->distBone[0] << std::endl;
			i++;
			if(c->center->edge)
			{
				Vector3f p[8];
				p[0] = c->vertices[7].position;
				p[1] = c->vertices[6].position;
				p[2] = c->vertices[5].position;
				p[3] = c->vertices[4].position;
				p[4] = c->vertices[3].position;
				p[5] = c->vertices[2].position;
				p[6] = c->vertices[1].position;
				p[7] = c->vertices[0].position;

				// Bottom face
				glNormal((p[3]-p[7]).CrossProduct(p[6]-p[7]));

				glVertex(p[7]);
				glVertex(p[3]);
				glVertex(p[2]);
				glVertex(p[6]);

				// Top face
				glNormal((p[4]-p[5]).CrossProduct(p[1]-p[5]));

				glVertex(p[5]);
				glVertex(p[4]);
				glVertex(p[0]);
				glVertex(p[1]);

				// Far face
				glNormal((p[5]-p[7]).CrossProduct(p[3]-p[7]));

				glVertex(p[7]);
				glVertex(p[5]);
				glVertex(p[1]);
				glVertex(p[3]);

				// Right face
				glNormal((p[1]-p[3]).CrossProduct(p[2]-p[3]));

				glVertex(p[3]);
				glVertex(p[1]);
				glVertex(p[0]);
				glVertex(p[2]);

				// Front face
				glNormal((p[2]-p[6]).CrossProduct(p[4]-p[6]));

				glVertex(p[6]);
				glVertex(p[2]);
				glVertex(p[0]);
				glVertex(p[4]);

				// Left Face
				glNormal((p[6]-p[7]).CrossProduct(p[5]-p[7]));

				glVertex(p[7]);
				glVertex(p[6]);
				glVertex(p[4]);
				glVertex(p[5]);
			}
		}
	}
	glEnd();

	glEnable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_DEPTH_TEST);
	glPolygonOffset(1.0, 1.0);

	// Render the lines around the cubes
	glColor3f(0.0f, 0.0f, 0.0f);
	
	glLineWidth(1.0f);
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
	for each(Body *body in bodies)
	{
		for each(Cell *c in body->cells)
		{
			if(c->center->edge)
			{
				Vector3f p[8];
				p[0] = c->vertices[7].position;
				p[1] = c->vertices[6].position;
				p[2] = c->vertices[5].position;
				p[3] = c->vertices[4].position;
				p[4] = c->vertices[3].position;
				p[5] = c->vertices[2].position;
				p[6] = c->vertices[1].position;
				p[7] = c->vertices[0].position;

				// Bottom face.
				glVertex(p[7]);
				glVertex(p[3]);
				glVertex(p[3]);
				glVertex(p[2]);
				glVertex(p[2]);
				glVertex(p[6]);
				glVertex(p[6]);
				glVertex(p[7]);

				// Top face.
				glVertex(p[5]);
				glVertex(p[4]);
				glVertex(p[4]);
				glVertex(p[0]);
				glVertex(p[0]);
				glVertex(p[1]);
				glVertex(p[1]);
				glVertex(p[5]);

				// Far face.
				glVertex(p[7]);
				glVertex(p[5]);
				glVertex(p[5]);
				glVertex(p[1]);
				glVertex(p[1]);
				glVertex(p[3]);
				glVertex(p[3]);
				glVertex(p[7]);

				// Right face.
				glVertex(p[3]);
				glVertex(p[1]);
				glVertex(p[1]);
				glVertex(p[0]);
				glVertex(p[0]);
				glVertex(p[2]);
				glVertex(p[2]);
				glVertex(p[3]);

				// Front face.
				glVertex(p[6]);
				glVertex(p[2]);
				glVertex(p[2]);
				glVertex(p[0]);
				glVertex(p[0]);
				glVertex(p[4]);
				glVertex(p[4]);
				glVertex(p[6]);

				// Left Face.
				glVertex(p[7]);
				glVertex(p[6]);
				glVertex(p[6]);
				glVertex(p[4]);
				glVertex(p[4]);
				glVertex(p[5]);
				glVertex(p[5]);
				glVertex(p[7]);
			}
		}
	}
	glEnd();
	glEnable(GL_LIGHTING);
}

//--------------------------------------------------------------
void Testbed::Render3(string _layerName)
{
	//ofSetSphereResolution(10);
	for each(Body *body in bodies){
		for each(Particle *p in body->particles){
			ofSetColor(255,0,0);
			if(_layerName == "R"){
				ofSphere(p->g.x, p->g.y, p->g.z, 4.0);
			}else if(_layerName == "B"){
				if(p->material.bone == true){
					ofSetColor(255);
					ofSphere(p->g.x, p->g.y, p->g.z, 4.0);
				}
			}else if(_layerName == "M"){
				if(p->material.muscle == true){
					ofSetColor(255, 0, 0);
					ofSphere(p->g.x, p->g.y, p->g.z, 4.0);
				}
			}else if(_layerName == "F"){
				if(p->material.fat == true){
					ofSetColor(255, 128, 0);
					ofSphere(p->g.x, p->g.y, p->g.z, 4.0);
				}
			}else if(_layerName == "S"){
				if(p->material.skin == true){
					ofSetColor(255, 255, 0);
					ofSphere(p->g.x, p->g.y, p->g.z, 4.0);
				}
			}else{
				ofSphere(p->g.x, p->g.y, p->g.z, 4.0);
			}
		}
	}
}

//--------------------------------------------------------------
void Testbed::Render4(float fatRatio)
{
	for each(Body *body in bodies)
	{
		for each(LatticeLocation *l in body->latticeLocations)
		{
			//if(l->material.bone == false && l->material.skin == false)
			//{
				// Muscle	// 依存する関節からの距離
				float value = ofMap(l->distBone[l->boneID], 0.0, body->LargestDist[l->boneID], 0.0, 1.0, false);
				//std::cout << value << "/" << fatRatio << endl;
				if(value < fatRatio)
				{
					l->material.muscle = true;
					l->particle->material.muscle = true;
					l->material.fat = false;
					l->particle->material.fat = false;
				}
				// Fat
				else{
					l->material.fat = true;
					l->particle->material.fat = true;
					l->material.muscle = false;
					l->particle->material.muscle = false;
				}
			//}
		}
	}
}

void Testbed::Render5(string _layerName)
{
	//ofSetSphereResolution(10);
	for each(Body *body in bodies){
		for each(Particle *p in body->particles){
			if(p->x.z<35)
			{
				glEnable(GL_DEPTH_TEST);

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL );
				if(p->material.bone == true){
					ofSetColor(255);
					ofBox(p->g.x, p->g.y, p->g.z, 10.0);
					//ofSphere(p->g.x, p->g.y, p->g.z, 5.0);
				}
				if(p->material.muscle == true){
					ofSetColor(255, 0, 0);
					ofBox(p->g.x, p->g.y, p->g.z, 10.0);
					//ofSphere(p->g.x, p->g.y, p->g.z, 5.0);
				}
				if(p->material.fat == true){
					ofSetColor(0, 0, 255);
					ofBox(p->g.x, p->g.y, p->g.z, 10.0);
					//ofSphere(p->g.x, p->g.y, p->g.z, 5.0);
				}
				if(p->material.skin == true){
					ofSetColor(234,147,149);
					ofBox(p->g.x, p->g.y, p->g.z, 10.0);
					//ofSphere(p->g.x, p->g.y, p->g.z, 5.0);
				}
				
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				ofSetLineWidth(2.0);
				ofSetColor(0);
				ofBox(p->g.x, p->g.y, p->g.z, 10.0);
			}
		}
	}
}

void Testbed::SetLayer(float fatRatio, int boneIndex, float m_stiff, float m_damp, float f_stiff, float f_damp)
{
	for each(Body *body in bodies)
	{
		for each(LatticeLocation *l in body->latticeLocations)
		{
			//if(l->material.bone == false && l->material.skin == false)
			//{
				if(l->boneID == boneIndex)
				{
					// Muscle	// 依存する関節からの距離
					//std::cout << l->boneID << std::endl;
					float value = ofMap(l->distBone[l->boneID], 0.0, body->LargestDist[l->boneID], 0.0, 1.0, false);

					l->material.muscle = false;
					l->particle->material.muscle = false;
					l->material.fat = false;
					l->particle->material.fat = false;

					if(value < fatRatio)
					{
						l->material.muscle = true;
						l->particle->material.muscle = true;
						l->material.fat = false;
						l->particle->material.fat = false;
						l->particle->alpha = m_stiff;
					}
					// Fat
					else if(l->material.skin == false){
						l->material.fat = true;
						l->particle->material.fat = true;
						l->material.muscle = false;
						l->particle->material.muscle = false;
					}
				}
			//}
		}
	}
}

void Testbed::RenderParticle(int boneNum, bool disp)
{
	//ofScale(1,0.5,1);
	if(disp)
	{
		//ofSetSphereResolution(10);
		for each(Body *body in bodies)
		{
			for each(LatticeLocation *l in body->latticeLocations)			
			//for each(Particle *particle in body->particles)
			{
				
				//if(	particle->dependenceJointIndex == boneNum )
				//{
					//float col;
					//float dist =l->distance;
					//col = ofMap(dist, 0, body->maxDistance, 0, 255);
					//VERTEX3D error = ErrorToRGB(col, 0.0, 255);
					//ofSetColor(error.x, error.y, error.z);

					if(l->_touch == true)
					{
						//ofSetColor(255, 0, 0);
					}else{
						//ofSetColor(0, 0, 255, 20);
						glColor4f(1.0f, 0.763f, 0.917f, 0.1);
					}

					//float col;
					//float dist =l->particle->responce.length() * (l->particle->dependenceJointIndex + 1);
					//col = ofMap(dist, 0, 500.0f, 0, 255);
					//VERTEX3D error = ErrorToRGB(col, 0.0, 255);
					//ofSetColor(error.x, error.y, error.z, 20+dist*10);
					ofSphere(ofVec3f(l->particle->g.x, l->particle->g.y, l->particle->g.z),5);
					//ofSphere(ofVec3f(l->particle->x.x, l->particle->x.y, l->particle->x.z),2);
				//}
			}
		}
	}
}

void Testbed::RenderForceDirection(ofxBvh *bvh, bool condition, bool renderExteanalForce)
{
	for each(Body *body in bodies)
	{
		if(condition){}
			//body->CalcExternalForce(bvh);
	
		if(renderExteanalForce)
		{
			for each(LatticeLocation *l in body->latticeLocations)
			{
				//Vector3f n(l->normal.x*10*l->scale, l->normal.y*10*l->scale, l->normal.z*10*l->scale);
				//float c_volume = l->vvv - 0.3125;
				//n = l->particle->R * n * c_volume;
				//ofVec3f direction(n.x, n.y, n.z);
				//l->lamda_ = 0.1f;
				//l->d *= l->lamda_;

				//ofVec3f direction(l->particle->responce);
				//ofVec3f temp(l->particle->f.x, l->particle->f.y, l->particle->f.z);
				ofVec3f temp(l->lamdaD);
				//std::cout << temp << endl;
				ofVec3f direction(temp);
				ofVec3f start(l->particle->g.x, l->particle->g.y, l->particle->g.z);
				ofVec3f end(start+direction);
				ofSetColor(255);

				VERTEX3D c = ErrorToRGB((end-start).length(), 0, 1.0);
				ofSetColor(c.x, c.y, c.z);
				ofSetLineWidth(2);
				ofDrawArrow(start, end, 1.0);
			}
		}
	}
}

void Testbed::RenderSkin(ObjModel *surface, string colorName)
{
	// Draw the cells
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);

	glMaterialf(GL_FRONT, GL_SHININESS, 10);//28
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	//gluLookAt(-1, 0, 0, 0, 0, 0, 0, 1, 0);

	if(colorName == "PINK")	{
		glColor4f(1.0f, 0.763f, 0.917f, 1.0);
	}else if(colorName == "PURPLE"){
		glColor4f(0.753f, 0.764f, 0.910f, 1.0);
	}else if(colorName == "YELLO"){
		glColor4f(0.997f, 1.0f, 0.763f, 1.0);
	}else if(colorName == "BLUE"){
		glColor4f(0.763f, 0.786f, 1.0f, 1.0);
	}

	for each(Body *body in bodies)
	{
		body->UpdateCellPositions();

		for each(Cell *c in body->cells)
		{
			Vector3f p[8];
			p[0] = c->vertices[7].position;
			p[1] = c->vertices[6].position;
			p[2] = c->vertices[5].position;
			p[3] = c->vertices[4].position;
			p[4] = c->vertices[3].position;
			p[5] = c->vertices[2].position;
			p[6] = c->vertices[1].position;
			p[7] = c->vertices[0].position;

			for each(Skin s in c->skins)
			{
				Proportion pro = s.interior;

				Vector3f _p = Vector3f(	(1-pro.u) * (1-pro.t) * (1-pro.s) * p[7] +
										(1-pro.u) * (1-pro.t) *   pro.s   * p[3] + 
										(1-pro.u) *   pro.t   * (1-pro.s) * p[6] +
										(1-pro.u) *   pro.t   *   pro.s   * p[2] +
										  pro.u   * (1-pro.t) * (1-pro.s) * p[5] +
										  pro.u   * (1-pro.t) *   pro.s   * p[1] +
										  pro.u   *   pro.t   * (1-pro.s) * p[4] +
										  pro.u   *   pro.t   *   pro.s   * p[0]
									 );
				
				surface->SetVertex(s.index, VERTEX3D(_p.x, _p.y, _p.z));
			}
		}
	}
	surface->CalcNormal();
	surface->RenderPoly(1.0);
}

VERTEX3D Testbed::ErrorToRGB(float err, float errMin, float errMax)
{
	int r, g, b;
	float norm_err = (err - errMin) / (errMax - errMin);
	float H, Hi, f, p, q, t, S = 1.0f, V = 1.0f;

	// 誤差をHSV表色系からRGB値に変換する. 
	// 輝度と彩度はともに1.0とする. 

	//H = 360.0f - (360.0f * norm_err + 120.0f);
	H = 360.0f - (240.0f * norm_err + 120.0f);

	if(H < 0.0f) 
	{ 
		H = 0.0f; 
	}

	Hi = (float)floor(H / 60.0f);

	f = H / 60.0f - Hi;

	p = V * (1.0f - S);
	q = V * (1.0f - f * S);
	t = V * (1.0f - (1.0f - f) * S);

	r = g = b = 0;

	if(Hi == 0) 
	{
		r = (int)(255.0f * V); 
		g = (int)(255.0f * t);
		b = (int)(255.0f * p);
	}
	if(Hi == 1)
	{
		r = (int)(255.0f * q); 
		g = (int)(255.0f * V);
		b = (int)(255.0f * p);
	}
	if(Hi == 2)
	{
		r = (int)(255.0f * p); 
		g = (int)(255.0f * V);
		b = (int)(255.0f * t);
	}
	if(Hi == 3)
	{
		r = (int)(255.0f * p); 
		g = (int)(255.0f * q);
		b = (int)(255.0f * V);
	}
	if(Hi == 4)
	{
		r = (int)(255.0f * t); 
		g = (int)(255.0f * p);
		b = (int)(255.0f * V);
	}
	if(Hi == 5)
	{
		r = (int)(255.0f * V); 
		g = (int)(255.0f * p);
		b = (int)(255.0f * q);
	}

	VERTEX3D error(r,g,b);
	return error;
}

float Testbed::CalcVolume(ObjModel *surface)
{
	float volume;
	for each(Body *body in bodies)
	{
		volume = body->setUpMeshVolumePreserving(surface);
	}
	return volume;
}

ofVec3f Testbed::PointPick(int index, int mouse_x, int mouse_y, ofEasyCam cam)
{
	ofVec3f nearestVertex;;

	Vector3f v;
	for each(Body *body in bodies)
	{
		v = body->latticeLocations.at(index)->particle->g;
		nearestVertex = cam.worldToScreen(ofVec3f(v.x, v.y, v.z));
	}

	ofVec3f AxisX = cam.getXAxis();		// カメラのX軸取得
	ofVec3f AxisY = cam.getYAxis();	

	for each(Body *body in bodies)
	{
		ofVec3f Fext;
		Fext.x+=0.1*(AxisX.x*(mouse_x - nearestVertex.x) + (-1.0)*AxisY.x *(mouse_y - nearestVertex.y));
		Fext.y+=0.1*(AxisX.y*(mouse_x - nearestVertex.x) + (-1.0)*AxisY.y *(mouse_y - nearestVertex.y));
		Fext.z+=0.1*(AxisX.z*(mouse_x - nearestVertex.x) + (-1.0)*AxisY.z *(mouse_y - nearestVertex.y));
		body->latticeLocations.at(index)->particle->f += Vector3f(Fext.x, Fext.y, Fext.z);
		Vector3f f = body->latticeLocations.at(index)->particle->f;
		Vector3f p = body->latticeLocations.at(index)->particle->x;
		float sigma = 2.0f;
		for each(LatticeLocation *l in body->latticeLocations){
			double dist =sqrt((l->particle->x.x - p.x) * (l->particle->x.x - p.x)
							 +(l->particle->x.y - p.y) * (l->particle->x.y - p.y)
							 +(l->particle->x.z - p.z) * (l->particle->x.z - p.z)
							 );

			// ガウス関数
			l->particle->x.x += f.x * exp(-dist/(2*sigma*sigma)) * 100;
			l->particle->x.y += f.y * exp(-dist/(2*sigma*sigma)) * 100;
			l->particle->x.z += f.z * exp(-dist/(2*sigma*sigma)) * 100;

			//std::cout << l->particle->f.x << "," << l->particle->f.y << "," << l->particle->f.z << std::endl;
		}
	}
	return nearestVertex;
}

int Testbed::getIndexPickingPoint(int mouse_x, int mouse_y, ofEasyCam cam)
{
	float nearestDistance = 10000000;
	ofVec2f mouse(mouse_x, mouse_y);
	ofVec3f temp;
	int number;

	for each(Body *body in bodies)
	{
		int i=0;
		for each(LatticeLocation *l in body->latticeLocations)
		{
			temp.set(l->particle->g.x, l->particle->g.y, l->particle->g.z);
			ofVec2f cur = cam.worldToScreen(temp);
			float distance = cur.distance(mouse);
			if(distance < nearestDistance) {
				nearestDistance = distance;
				number = i;
			}
			i++;
		}
	}
	
	return number;
}

ofVec3f Testbed::getVertex(int index)
{
	Vector3f v;
	for each(Body *body in bodies)
	{
		v = body->latticeLocations.at(index)->particle->g;
	}

	return ofVec3f(v.x, v.y, v.z);
}