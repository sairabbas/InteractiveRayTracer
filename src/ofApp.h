//Sair Abbas - CS116

#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "box.h"
#include "glm/gtx/intersect.hpp"
#include "glm/gtx/euler_angles.hpp"

//  General Purpose Ray class 
//
class Ray {
public:
	Ray(glm::vec3 p, glm::vec3 d) { this->p = p; this->d = d; }
	void draw(float t) { ofDrawLine(p, p + t * d); }

	glm::vec3 evalPoint(float t) {
		return (p + t * d);
	}
	glm::vec3 p, d;
};

//  Base class for any renderable object in the scene
//
class SceneObject {
public:
	virtual void draw() = 0;   
	virtual bool intersect(const Ray &ray, glm::vec3 &point, glm::vec3 &normal) { return false; }
	virtual bool lightIntersect(const Ray &ray, glm::vec3 &point, glm::vec3 &normal) { return false; }
	glm::mat4 getRotateMatrix() {
		return (glm::eulerAngleYXZ(glm::radians(rotation.y), glm::radians(rotation.x), glm::radians(rotation.z)));  
	}
	glm::mat4 getTranslateMatrix() {
		return (glm::translate(glm::mat4(1.0), glm::vec3(position.x, position.y, position.z)));
	}
	glm::mat4 getMatrix() {
		glm::mat4 rotate = getRotateMatrix();
		glm::mat4 trans = getTranslateMatrix();
		return (trans * rotate);
	}
	glm::vec3 getPosition() {
		return (getMatrix() * glm::vec4(0.0, 0.0, 0.0, 1.0));
	}
	void setPosition(glm::vec3 pos) {
		position = glm::inverse(getMatrix()) * glm::vec4(pos, 1.0);
	}
	glm::vec3 position = glm::vec3(0, 0, 0);   
	glm::vec3 rotation = glm::vec3(0, 0, 0);  
	ofColor diffuseColor = ofColor::grey;    
	ofColor specularColor = ofColor::lightGray;
	bool isSelectable = true;
};

//  General purpose sphere  (assume parametric)
//
class Sphere : public SceneObject {
public:
	Sphere(glm::vec3 p, float r, ofColor diffuse = ofColor::lightGray) { 
		position = p; 
		radius = r; 
		diffuseColor = diffuse; 
	}
	Sphere() {}
	bool intersect(const Ray &ray, glm::vec3 &point, glm::vec3 &normal) {
		glm::mat4 mInv = glm::inverse(getMatrix());
		glm::vec4 p = mInv * glm::vec4(ray.p.x, ray.p.y, ray.p.z, 1.0);
		glm::vec4 p1 = mInv * glm::vec4(ray.p + ray.d, 1.0);
		glm::vec3 d = glm::normalize(p1 - p);
		return (glm::intersectRaySphere(glm::vec3(p), d, glm::vec3(0, 0, 0), radius, point, normal));
	}
	void draw() {
		glm::mat4 m = getMatrix();
		ofPushMatrix();
		ofMultMatrix(m);
		ofDrawSphere(radius);
		ofPopMatrix();
	}
	float radius = 1.0;
};

//  General purpose plane 
//
class Plane : public SceneObject {
public:
	Plane(glm::vec3 p, glm::vec3 n, float w = 20, float h = 20, ofColor color = ofColor::darkSlateGray) {
		position = p;
		normal = n;
		width = w;
		height = h;
		diffuseColor = color;
		plane.setWidth(width);
		plane.setHeight(height);
		plane.setResolution(4, 4);
		if (normal == glm::vec3(0, 1, 0))
			plane.rotateDeg(90, 1, 0, 0);
	}
	Plane() {
		normal = glm::vec3(0, 1, 0);
		plane.rotateDeg(90, 1, 0, 0);
	}
	bool intersect(const Ray &ray, glm::vec3 & point, glm::vec3 & normal);
	glm::vec3 getNormal(const glm::vec3 &p) {
		return this->normal;
	}
	void draw() {
		glm::mat4 m = getMatrix();
		ofPushMatrix();
		ofMultMatrix(m);
		plane.drawFaces();
		ofPopMatrix();
	}
	ofPlanePrimitive plane;
	glm::vec3 normal;
	float width = 20;
	float height = 20;
};

// View plane for render camera
// 
class  ViewPlane : public Plane {
public:
	ViewPlane(glm::vec2 p0, glm::vec2 p1) { min = p0; max = p1; }
	ViewPlane() {                         // create reasonable defaults (6x4 aspect)
		min = glm::vec2(-3, -2);
		max = glm::vec2(3, 2);
		position = glm::vec3(0, 0, 5);
		normal = glm::vec3(0, 0, 1);      // viewplane currently limited to Z axis orientation
		isSelectable = false;
	}
	void setSize(glm::vec2 min, glm::vec2 max) { this->min = min; this->max = max; }
	float getAspect() { return width() / height(); }
	glm::vec3 toWorld(float u, float v);   //   (u, v) --> (x, y, z) [ world space ]
	void draw() {
		ofSetColor(diffuseColor);
		ofDrawRectangle(glm::vec3(min.x, min.y, position.z), width(), height());
	}
	float width() {
		return (max.x - min.x);
	}
	float height() {
		return (max.y - min.y);
	}
	glm::vec2 topLeft() { return glm::vec2(min.x, max.y); }
	glm::vec2 topRight() { return max; }
	glm::vec2 bottomLeft() { return min; }
	glm::vec2 bottomRight() { return glm::vec2(max.x, min.y); }
	glm::vec2 min, max;
};

//  render camera  - currently must be z axis aligned (we will improve this in project 4)
//
class RenderCam : public SceneObject {
public:
	RenderCam() {
		position = glm::vec3(0, 0, 10);
		aim = glm::vec3(0, 0, -1);
		isSelectable = false;
	}
	Ray getRay(float u, float v);
	void draw() { ofDrawBox(position, 1.0); };
	glm::vec3 aim;
	ViewPlane view;          // The camera viewplane, this is the view that we will render 
};

//General purpose point light
class PointLight : public SceneObject {
public:
	float intensity, radius = 0.8;
	PointLight() {}
	PointLight(glm::vec3 p, float i, ofColor color = ofColor::darkBlue) {
		position = p;
		intensity = i;
		diffuseColor = color;
	}
	void draw() {
		glm::mat4 m = getMatrix();
		ofPushMatrix();
		ofMultMatrix(m);
		ofDrawSphere(radius);
		ofPopMatrix();
	}
	bool lightIntersect(const Ray &ray, glm::vec3 &point, glm::vec3 &normal) {
		return (glm::intersectRaySphere(ray.p, ray.d, position, radius, point, normal));
	}
};

class SpotLight : public SceneObject {
public:
	float intensity, radius = 0.8, height = 2;
	glm::vec3 aim;
	SpotLight(glm::vec3 p, float i, ofColor color = ofColor::darkRed) {
		position = p;
		intensity = i;
		diffuseColor = color;
	}
	void draw() {
		glm::mat4 m = lookAtMatrix(position, aim, glm::vec3(0, 1, 0));
		ofPushMatrix();
		ofMultMatrix(m);
		ofRotate(-90, 1, 0, 0);
		ofDrawCone(radius, height);
		ofPopMatrix();
	}
	glm::mat4 lookAtMatrix(const glm::vec3 &pos, const glm::vec3 &aimPos, glm::vec3 upVector) {
		glm::mat4 m;
		glm::vec3 dir = glm::normalize(pos - aimPos);
		glm::vec3 right = glm::normalize(glm::cross(upVector, dir));
		glm::vec3 newUp = glm::cross(dir, right);
		m[0][0] = right.x;
		m[0][1] = right.y;
		m[0][2] = right.z;
		m[0][3] = 0;
		m[1][0] = newUp.x;
		m[1][1] = newUp.y;
		m[1][2] = newUp.z;
		m[1][3] = 0;
		m[2][0] = dir.x;
		m[2][1] = dir.y;
		m[2][2] = dir.z;
		m[2][3] = 0;
		m[3][0] = pos.x;
		m[3][1] = pos.y;
		m[3][2] = pos.z;
		m[3][3] = 1;
		return m;
	}
	bool lightIntersect(const Ray &ray, glm::vec3 &point, glm::vec3 &normal) {
		glm::mat4 mInv = glm::inverse(getMatrix());
		glm::vec4 p = mInv * glm::vec4(ray.p.x, ray.p.y, ray.p.z, 1.0);
		glm::vec4 p1 = mInv * glm::vec4(ray.p + ray.d, 1.0);
		glm::vec3 d = glm::normalize(p1 - p);
		_Ray boxRay = _Ray(Vector3(p.x, p.y, p.z), Vector3(d.x, d.y, d.z));
		Box box = Box(Vector3(-radius, -radius, 0), Vector3(radius, radius, height));
		return (box.intersect(boxRay, -1000, 1000));
	}
};

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	void rayTrace();
	void createSphere();
	void createPlane();
	void createPointLight();
	void createSpotLight();
	void deleteObject();

	glm::vec3 lastPoint;
	bool bRotateX = false;
	bool bRotateY = false;
	bool bRotateZ = false;
	bool bDrag = false;
	bool objSelected() { return (selected.size() ? true : false); };
	bool mouseToDragPlane(int x, int y, glm::vec3 &point);
	bool toggleShading = false;
	bool bHide = true;
	bool bShowImage = false;
	bool renderFinish = false;
	bool insideShadow(const Ray shadowRay);

	ofColor lambert(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse);
	ofColor phong(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse, const ofColor specular, float power);

	ofEasyCam  mainCam;
	ofCamera sideCam;
	ofCamera previewCam;
	ofCamera  *theCam;    
	RenderCam renderCam;
	ofImage image;

	vector<SceneObject *> scene;
	vector<PointLight *> pointLights;
	vector<SpotLight *> spotLights;
	vector<SceneObject *> selected;

	int imageWidth = 1200;
	int imageHeight = 800;

	ofxFloatSlider power;
	ofxFloatSlider pointIntensity;
	ofxFloatSlider spotIntensity;
	ofxFloatSlider spotSize;
	ofxVec3Slider spotAim;
	ofxPanel gui;
};
