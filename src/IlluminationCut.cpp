#include "IlluminationCut.h"
#include "raytracer.h"


LightTree::LightTree(std::vector<point_light_t>& vpls) {
	this->root = new LightNode(vpls, true);
};

void LightTree::cluster() {
	this->root->greedy();
}

PointTree::PointTree() {
	root = new PointNode();
}

void PointTree::cluster() {
	root->voxelize();
}