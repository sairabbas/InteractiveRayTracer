//Sair Abbas - CS116

#include "ofApp.h"

// Intersect Ray with Plane  (wrapper on glm::intersect*
//
bool Plane::intersect(const Ray &ray, glm::vec3 & point, glm::vec3 & normalAtIntersect) {
	float dist;
	bool insidePlane = false;
	bool hit = glm::intersectRayPlane(ray.p, ray.d, position, this->normal, dist);
	if (hit) {
		Ray r = ray;
		point = r.evalPoint(dist);
		normalAtIntersect = this->normal;
		glm::vec2 xrange = glm::vec2(position.x - width / 2, position.x + width / 2);
		glm::vec2 zrange = glm::vec2(position.z - height / 2, position.z + height / 2);
		if (point.x < xrange[1] && point.x > xrange[0] && point.z < zrange[1] && point.z > zrange[0]) {
			insidePlane = true;
		}
	}
	return insidePlane;
}

// Convert (u, v) to (x, y, z) 
// We assume u,v is in [0, 1]
//
glm::vec3 ViewPlane::toWorld(float u, float v) {
	float w = width();
	float h = height();
	return (glm::vec3((u * w) + min.x, (v * h) + min.y, position.z));
}

// Get a ray from the current camera position to the (u, v) position on
// the ViewPlane
//
Ray RenderCam::getRay(float u, float v) {
	glm::vec3 pointOnPlane = view.toWorld(u, v);
	return(Ray(position, glm::normalize(pointOnPlane - position)));
}

//Raytracing function
void ofApp::rayTrace() {
    //Begin render
	ofColor color;
    cout << "Rendering..." << endl;
    //Iterate through each pixel
	for (int j = 0; j < image.getHeight(); j++)
	{
		for (int i = 0; i < image.getWidth(); i++)
		{
            //Initialize variables
			float u = (i + 0.5) / image.getWidth();
			float v = (j + 0.5) / image.getHeight();
			float currentDist, closestDist = std::numeric_limits<float>::infinity();
			Ray ray = renderCam.getRay(u, v);
			bool hit = false;
			SceneObject *closestObject = NULL;
			glm::vec3 intersectPoint, normal, closestIntersect, closestNormal;
            //Iterate through each scene object
			for (int a = 0; a < scene.size(); a++)
			{
                //Check if ray intersected with scene object
				if (scene[a]->intersect(ray, intersectPoint, normal))
				{
					hit = true;
					currentDist = glm::length(intersectPoint - renderCam.position);
                    //If closest object assign values to variables
					if (currentDist < closestDist)
					{
						closestIntersect = intersectPoint;
						closestNormal = normal;
						closestDist = currentDist;
						closestObject = scene[a];
					}
				}
				//If closest object color pixel same as object
				if (hit == true)
				{
					//Toggle shaders
					if (toggleShading)
						color = phong(closestIntersect, closestNormal, closestObject->diffuseColor, closestObject->specularColor, power);
					else
						color = lambert(closestIntersect, closestNormal, closestObject->diffuseColor);
					image.setColor(i, j, color);
				}
				//If no intersect color pixel as background
				else
				{
					image.setColor(i, j, ofGetBackgroundColor());
				}
			}
		}
	}
    //Save image right side up
    image.mirror(true, false);
	image.save("image.png");
    //Confirm render as complete
    cout << "Finished" << endl << endl;
	renderFinish = true;
}

//Lambert shading function
ofColor ofApp::lambert(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse) {
	//Set ambient 
	ofColor color = diffuse * 0.25;
	//Point light shading
	for (int i = 0; i < pointLights.size(); i++)
	{
		//Calculate light source
		float radius = 1;
		float intensity = pointLights[i]->intensity;
		float lightSource = (intensity / (radius * radius));
		glm::vec3 l = normalize(pointLights[i]->position - p);
		glm::vec3 n = normalize(norm);
		Ray shadowRay = Ray(p + (n * 0.1), l);
		//Accumulate color
		if (insideShadow(shadowRay))
		{
			//Ambient shading
		}
		else
			color += diffuse * lightSource * max(float(0), dot(n, l));
	}
	//Spot light shading
	for (int i = 0; i < spotLights.size(); i++)
	{
		//Calculate light source
		float radius = 1;
		float intensity = spotLights[i]->intensity;
		float lightSource = (intensity / (radius * radius));
		glm::vec3 l = normalize(spotLights[i]->position - p);
		glm::vec3 n = normalize(norm);
		Ray shadowRay = Ray(p + (n * 0.1), l);
		//Calculate spot light direction
		glm::vec3 dir = normalize(spotLights[i]->position - spotLights[i]->aim);
		float angle = glm::dot(l, dir);
		angle = glm::acos(angle);
		//Accumulate color
		if (insideShadow(shadowRay))
		{
			//Ambient shading
		}
		else if (angle < spotSize)
		{
			color += diffuse * lightSource * max(float(0), dot(n, l));
		}
	}
	return color;
}

//Phong shading function
ofColor ofApp::phong(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse, const ofColor specular, float power) {
	//Set ambient 
	ofColor color = diffuse * 0.25;
	//Point light shading
	for (int i = 0; i < pointLights.size(); i++)
	{
		//Calculate light source
		float radius = 1;
		float intensity = pointLights[i]->intensity;
		float lightSource = (intensity / (radius * radius));
		glm::vec3 n = normalize(norm);
		glm::vec3 l = normalize(pointLights[i]->position - p);
		glm::vec3 v = normalize(renderCam.position - p);
		glm::vec3 h = normalize(v + l);
		Ray shadowRay = Ray(p + (n * 0.1), l);
		//Accumulate color
		if (insideShadow(shadowRay))
		{
			//Ambient shading
		}
		else
		{
			color += diffuse * lightSource * max(float(0), dot(n, l))
				+ specular * lightSource
				* pow(max(float(0), dot(n, h)), power);
		}
	}
	//Spot light shading
	for (int i = 0; i < spotLights.size(); i++)
	{
		//Calculate light source
		float radius = 1;
		float intensity = spotLights[i]->intensity;
		float lightSource = (intensity / (radius * radius));
		glm::vec3 n = normalize(norm);
		glm::vec3 l = normalize(spotLights[i]->position - p);
		glm::vec3 v = normalize(renderCam.position - p);
		glm::vec3 h = normalize(v + l);
		Ray shadowRay = Ray(p + (n * 0.1), l);
		//Calculate spot light direction
		glm::vec3 dir = normalize(spotLights[i]->position - spotLights[i]->aim);
		float angle = glm::dot(l, dir);
		angle = glm::acos(angle);
		//Accumulate color
		if (insideShadow(shadowRay))
		{
			//Ambient shading
		}
		else if (angle < spotSize)
		{
			color += diffuse * lightSource * max(float(0), dot(n, l))
				+ specular * lightSource
				* pow(max(float(0), dot(n, h)), power);
		}
	}
	return color;
}

//Check if inside shadow function
bool ofApp::insideShadow(const Ray shadowRay) {
	glm::vec3 intersectPoint, normal;
	for (int i = 0; i < scene.size(); i++)
	{
		if (scene[i]->intersect(shadowRay, intersectPoint, normal))
		{
			return true;
		}
	}
	return false;
}

//--------------------------------------------------------------
void ofApp::setup(){
	//Set GUI
	gui.setup();
	gui.add(power.setup("Power", 100, 1, 100));
	gui.add(pointIntensity.setup("Point Intensity", 1, 0.1, 5));
	gui.add(spotIntensity.setup("Spot Intensity", 1, 0.1, 5));
	gui.add(spotSize.setup("Spot Size ", 0.3, 0.1, 0.9));
	gui.add(spotAim.setup("Spot Aim", glm::vec3(0, 0, 0), glm::vec3(-10, -10, -10), glm::vec3(10, 10, 10)));
	
	//Allocate image
	image.allocate(imageWidth, imageHeight, ofImageType::OF_IMAGE_COLOR);
    //Set background
	ofSetBackgroundColor(ofColor::black);
    //Set cameras
	theCam = &mainCam;
	mainCam.setDistance(15);
    sideCam.setPosition(15, 0, -1);
    sideCam.lookAt(glm::vec3(0, 0, 0));
	previewCam.setPosition(renderCam.position);
    previewCam.setFov(45);
    //Initialize scene objects
    scene.push_back(new Sphere(glm::vec3(0, 0, 0), 1.5, ofColor::darkSeaGreen));
	scene.push_back(new Plane(glm::vec3(0, -1.5, 0), glm::vec3(0, 1, 0), 20, 20, ofColor::darkSlateGray));
	//Initialize Lights
	PointLight* pointLight = new PointLight(glm::vec3(3, 6, 4), pointIntensity, ofColor::darkRed);
	pointLights.push_back(pointLight);
	scene.push_back(pointLight);
	SpotLight* spotLight = new SpotLight(glm::vec3(-0.01, 6, 0), spotIntensity, ofColor::darkBlue);
	spotLights.push_back(spotLight);
	scene.push_back(spotLight);
}

//--------------------------------------------------------------
void ofApp::update(){
	//Update each point light
	for (int i = 0; i < pointLights.size(); i++)
	{
		//Update point light intensity
		if (pointLights.size() == 1) 
		{
			pointLights[0]->intensity = pointIntensity;
		}
		else if (objSelected() && pointLights[i] == selected[0])
		{
			pointLights[i]->intensity = pointIntensity;
		}
	}
	//Update each spot light
	for (int i = 0; i < spotLights.size(); i++)
	{
		//Update spot light intensity and aim
		if (spotLights.size() == 1)
		{
			spotLights[0]->intensity = spotIntensity;
			spotLights[0]->aim = spotAim;
		}
		else if (objSelected() && spotLights[i] == selected[0])
		{
			spotLights[i]->intensity = spotIntensity;
			spotLights[i]->aim = spotAim;
		}
	}
}

//-------------------------------------------------------------- 
void ofApp::draw(){
	ofEnableDepthTest();
	theCam->begin();
	//Draw each scene object
	for (int i = 0; i < scene.size(); i++) {
		if (objSelected() && scene[i] == selected[0])
			ofSetColor(ofColor::white);
		else ofSetColor(scene[i]->diffuseColor);
		scene[i]->draw();
	}
    //Draw view plane
	renderCam.view.draw();
	theCam->end();
	ofDisableDepthTest();
	//Inform input toggle
	ofSetColor(ofColor::white);
	if (mainCam.getMouseInputEnabled())
		ofDrawBitmapString("Input: Disabled", ofGetWindowWidth() - 150, 25);
	else
		ofDrawBitmapString("Input: Enabled", ofGetWindowWidth() - 150, 25);
	//Inform shading toggle
	if (toggleShading)
		ofDrawBitmapString("Shading: Phong", ofGetWindowWidth() - 150, 45);
	else
		ofDrawBitmapString("Shading: Lambert", ofGetWindowWidth() - 150, 45);
	//If rendering complete draw rendered image
	if (renderFinish == true)
		image.draw(0, 0);
	//Draw GUI
	gui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	switch (key) {
		//Toggle mouse input
	case 'c':
		if (mainCam.getMouseInputEnabled()) {
			mainCam.disableMouseInput();
		}
		else {
			mainCam.enableMouseInput();
			selected.clear();
		}
		break;
		//Delete object 
	case 'd':
		deleteObject();
		break;
		//Disable rendering
	case 'f':
		renderFinish = false;
		break;
		//Disable rendering
	case 'j':
		createPointLight();
		break;
		//Disable rendering
	case 'k':
		createSpotLight();
		break;
		//Create plane 
	case 'p':
		createPlane();
		break;
		//Toggle Lambert/Phong shading
	case 'q':
		toggleShading = !toggleShading;
		break;
		//Enable rendering
	case 'r':
		rayTrace();
		break;
		//Create sphere 
	case 's':
		createSphere();
		break;
		//Switch cameras
	case OF_KEY_F1:
		theCam = &mainCam;
		break;
	case OF_KEY_F2:
		theCam = &previewCam;
		break;
	case OF_KEY_F3:
		theCam = &sideCam;
		break;
		//Rotate by axis
	case 'x':
		bRotateX = true;
		break;
	case 'y':
		bRotateY = true;
		break;
	case 'z':
		bRotateZ = true;
		break;
	}
}

//Create sphere scene object
void ofApp::createSphere()
{
	//Declare return point variable
	glm::vec3 pointRtn = glm::vec3(0, 0, 0);
	//Project mouse point onto 3D point normal to the view axis 
	if (mouseToDragPlane(ofGetMouseX(), ofGetMouseY(), pointRtn) == true) {
		//Add new sphere 
		Sphere *temp = new Sphere(pointRtn, 1.5, ofColor::darkSeaGreen);
		scene.push_back(temp);
	}
}

//Create plane scene object
void ofApp::createPlane()
{
	//Declare return point variable
	glm::vec3 pointRtn = glm::vec3(0, 0, 0);
	//Project mouse point onto 3D point normal to the view axis 
	if (mouseToDragPlane(ofGetMouseX(), ofGetMouseY(), pointRtn) == true) {
		//Add new plane 
		Plane *temp = new Plane(pointRtn, glm::vec3(0, 1, 0), 20, 20, ofColor::darkSlateGray);
		scene.push_back(temp);
	}
}

//Create point light object
void ofApp::createPointLight()
{
	//Declare return point variable
	glm::vec3 pointRtn = glm::vec3(0, 0, 0);
	//Project mouse point onto 3D point normal to the view axis 
	if (mouseToDragPlane(ofGetMouseX(), ofGetMouseY(), pointRtn) == true) {
		//Add new point light 
		PointLight *temp = new PointLight(pointRtn, pointIntensity, ofColor::darkRed);
		scene.push_back(temp);
		pointLights.push_back(temp);
	}
}

//Create point light object
void ofApp::createSpotLight()
{
	//Declare return point variable
	glm::vec3 pointRtn = glm::vec3(0, 0, 0);
	//Project mouse point onto 3D point normal to the view axis 
	if (mouseToDragPlane(ofGetMouseX(), ofGetMouseY(), pointRtn) == true) {
		//Add new point light 
		SpotLight *temp = new SpotLight(pointRtn, spotIntensity, ofColor::darkBlue);
		scene.push_back(temp);
		spotLights.push_back(temp);
	}
}

//Delete object
void ofApp::deleteObject()
{
	if (objSelected()) {
		//Delete scene object
		for (int i = 0; i < scene.size(); i++) {
			if (scene[i] == selected[0])
				scene.erase(scene.begin() + i);
		}
		//Delete point light
		for (int i = 0; i < pointLights.size(); i++) {
			if (pointLights[i] == selected[0])
				pointLights.erase(pointLights.begin() + i);
		}
		//Delete spot light
		for (int i = 0; i < spotLights.size(); i++) {
			if (spotLights[i] == selected[0])
				spotLights.erase(spotLights.begin() + i);
		}
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
	switch (key) {
	case 'x':
		bRotateX = false;
		break;
	case 'y':
		bRotateY = false;
		break;
	case 'z':
		bRotateZ = false;
		break;
	}
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

	if (objSelected() && bDrag) {
		glm::vec3 point;
		mouseToDragPlane(x, y, point);
		if (bRotateX) {
			selected[0]->rotation += glm::vec3((point.x - lastPoint.x) * 20.0, 0, 0);
		}
		else if (bRotateY) {
			selected[0]->rotation += glm::vec3(0, (point.x - lastPoint.x) * 20.0, 0);
		}
		else if (bRotateZ) {
			selected[0]->rotation += glm::vec3(0, 0, (point.x - lastPoint.x) * 20.0);
		}
		else {
			selected[0]->position += (point - lastPoint);
		}
		lastPoint = point;
	}

}

//  This projects the mouse point in screen space (x, y) to a 3D point on a plane
//  normal to the view axis of the camera passing through the point of the selected object.
//  If no object selected, the plane passing through the world origin is used.
//
bool ofApp::mouseToDragPlane(int x, int y, glm::vec3 &point) {
	glm::vec3 p = theCam->screenToWorld(glm::vec3(x, y, 0));
	glm::vec3 d = p - theCam->getPosition();
	glm::vec3 dn = glm::normalize(d);

	float dist;
	glm::vec3 pos;
	if (objSelected()) {
		pos = selected[0]->position;
	}
	else pos = glm::vec3(0, 0, 0);
	if (glm::intersectRayPlane(p, dn, pos, glm::normalize(theCam->getZAxis()), dist)) {
		point = p + dn * dist;
		return true;
	}
	return false;
}

// Provides functionality of single selection and if something is already selected,
// sets up state for translation/rotation of object using mouse.
//
void ofApp::mousePressed(int x, int y, int button) {

	// if we are moving the camera around, don't allow selection
	//
	if (mainCam.getMouseInputEnabled()) return;

	// clear selection list
	//
	selected.clear();

	//
	// test if something selected
	//
	vector<SceneObject *> hits;

	glm::vec3 p = theCam->screenToWorld(glm::vec3(x, y, 0));
	glm::vec3 d = p - theCam->getPosition();
	glm::vec3 dn = glm::normalize(d);

	// check for selection of scene objects
	//
	for (int i = 0; i < scene.size(); i++) {

		glm::vec3 point, norm;

		//  We hit an object
		//
		if (scene[i]->isSelectable && scene[i]->intersect(Ray(p, dn), point, norm)) {
			hits.push_back(scene[i]);
		}
		else if (scene[i]->isSelectable && scene[i]->lightIntersect(Ray(p, dn), point, norm)) {
			hits.push_back(scene[i]);
		}
	}

	// if we selected more than one, pick nearest
	//
	SceneObject *selectedObj = NULL;
	if (hits.size() > 0) {
		selectedObj = hits[0];
		float nearestDist = std::numeric_limits<float>::infinity();
		for (int n = 0; n < hits.size(); n++) {
			float dist = glm::length(hits[n]->position - theCam->getPosition());
			if (dist < nearestDist) {
				nearestDist = dist;
				selectedObj = hits[n];
			}
		}
	}
	if (selectedObj) {
		selected.push_back(selectedObj);
		bDrag = true;
		mouseToDragPlane(x, y, lastPoint);
	}
	else {
		selected.clear();
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
