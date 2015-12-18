#pragma once

#include <vector>
#include <limits>
#include "raytracer.h"


class LightNode {
public:
	LightNode* parent;
	LightNode* left_child;
	LightNode* right_child;

	std::vector<point_light_t>* vpls;
	int rep_light;
	int min_light;
	std::vector<int> lights;
	float radius;

	LightNode(std::vector<point_light_t>& vpls, bool isRoot = false) {
		this->parent = this->left_child = this->right_child = NULL;
		this->vpls = &vpls;
		rep_light = -1;
		radius = 0.0;
		if (isRoot) {
			for (int i = 0; i < vpls.size(); i++)
				this->lights.push_back(i);
			find_rep_light();
		}
	}

	void greedy() {
		if(lights.size() > 1) {
			this->left_child = new LightNode(*vpls);
			this->right_child = new LightNode(*vpls);
			this->left_child->parent = this;
			this->right_child->parent = this;

			//random start index of greedy_cluster
			int first = std::rand() % lights.size();
			//get the farthest point from the start point
		
			int farthest = 0;
			for (int i = 1; i < lights.size(); i++) {
				if (glm::length((*vpls)[i].position - ((*vpls)[first].position)) > glm::length((*vpls)[farthest].position - ((*vpls)[first].position)))
					farthest = i;
			}
			first = farthest; //set as the center of first cluster

			int second = 0;
			for (int i = 1; i < lights.size(); i++) {
				if (glm::length((*vpls)[i].position - ((*vpls)[first].position)) > glm::length((*vpls)[second].position - ((*vpls)[first].position)))
					second = i;  //set as the center of second cluster
			}
			radius = glm::length((*vpls)[first].position - (*vpls)[second].position);

			this->left_child->lights.reserve(this->lights.size());
			this->right_child->lights.reserve(this->lights.size());
			this->left_child->lights.push_back(first);	//left child node as first cluster
			this->right_child->lights.push_back(second);	//right child node as second cluster

			for (int i = 0; i < lights.size(); i++) {
				if (i != first && i != second) {
					if (glm::length((*vpls)[i].position - ((*vpls)[first].position)) > glm::length((*vpls)[i].position - ((*vpls)[second].position))) {
						this->right_child->lights.push_back(i);
						//printf("right %d\n", this->right_child->lights.size());
					}
					else {
						this->left_child->lights.push_back(i);
						//printf("left %d\n", this->left_child->lights.size());
					}
				}
			}

			this->left_child->find_rep_light();
			this->right_child->find_rep_light();

			this->left_child->greedy();
			this->right_child->greedy();
		}
		else {
			rep_light = 0;
			min_light = 0;
		}
	}

	void find_rep_light() {
		if (lights.size() > 1) {
			// get the average of light clusters
			min_light = 0;
			glm::vec3 sum(0.0, 0.0, 0.0);
			for (int i = 0; i < lights.size(); i++) {
				sum += (*vpls)[lights[i]].direction * glm::length((*vpls)[lights[i]].intensity);
				min_light = glm::length((*vpls)[lights[i]].intensity) < glm::length((*vpls)[lights[min_light]].intensity) ? i : min_light;
			}
			sum /= lights.size();

			rep_light = 0;
			for (int i = 1; i < lights.size(); i++) {
				if (glm::length((*vpls)[lights[i]].direction * glm::length((*vpls)[lights[i]].intensity) - sum) < glm::length((*vpls)[lights[rep_light]].direction * glm::length((*vpls)[lights[rep_light]].intensity) - sum))
					rep_light = i;
			}
		}
	}
};


class LightTree {
public:
	LightNode* root;

	LightTree(std::vector<point_light_t>& vpls);

	void cluster();
};


class PointNode {
public:
	PointNode* parent;
	PointNode* children[8];
	/* order of 8 children :: Voxelized
	(1, 1, -1), (-1, 1, -1), (1, -1, -1), (-1, -1, -1),
	(1, 1, 1), (-1, 1, 1), (1, -1, 1), (-1, -1, 1)
	*/

	std::vector<glm::vec3> points;
	std::vector<glm::vec3> normals;
	float radius;

	PointNode() {
		parent = NULL;
		for (int i = 0; i < 8; i++) {
			children[i] = NULL;
		}
		points = std::vector<glm::vec3>();
		normals = std::vector<glm::vec3>();
		radius = 0.0;
	}

	void voxelize() {
		if (points.size() > 1) {
			// fisrt, find the smallest bounding box
			float min[3] = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
			float max[3] = { std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };
			for (int i = 0; i < points.size(); i++) {
				min[0] = min[0] > points[i].x ? points[i].x : min[0];
				min[1] = min[1] > points[i].y ? points[i].y : min[1];
				min[2] = min[2] > points[i].z ? points[i].z : min[2];

				max[0] = max[0] < points[i].x ? points[i].x : max[0];
				max[1] = max[1] < points[i].y ? points[i].y : max[1];
				max[2] = max[2] < points[i].z ? points[i].z : max[2];
			}
			radius = (max[0] - min[0])>(max[1] - min[1]) ? max[0] - min[0] : max[1] - min[1];
			radius = (max[2] - min[2])>radius ? max[2] - min[2] : radius;

			float division[3] = { (max[0] + min[0]) / 2., (max[1] + min[1]) / 2., (max[2] + min[2]) / 2. };

			for (int i = 0; i < points.size(); i++) {
				int child_index;
				if (points[i].x >= division[0] && points[i].y >= division[1] && points[i].z < division[2])
					child_index = 0;
				else if (points[i].x < division[0] && points[i].y >= division[1] && points[i].z < division[2])
					child_index = 1;
				else if (points[i].x >= division[0] && points[i].y < division[1] && points[i].z < division[2])
					child_index = 2;
				else if (points[i].x < division[0] && points[i].y < division[1] && points[i].z < division[2])
					child_index = 3;
				if (points[i].x >= division[0] && points[i].y >= division[1] && points[i].z >= division[2])
					child_index = 4;
				else if (points[i].x < division[0] && points[i].y >= division[1] && points[i].z >= division[2])
					child_index = 5;
				else if (points[i].x >= division[0] && points[i].y < division[1] && points[i].z >= division[2])
					child_index = 6;
				else if (points[i].x < division[0] && points[i].y < division[1] && points[i].z >= division[2])
					child_index = 7;

				if (children[child_index] == NULL) {
					children[child_index] = new PointNode();
					children[child_index]->parent = this;
				}
				children[child_index]->points.push_back(points[i]);
				children[child_index]->normals.push_back(normals[i]);
			}

			for (int i = 0; i < 8; i++) {
				if (children[i] != NULL)
					children[i]->voxelize();
			}
		}
	}
};

class PointTree {
public:
	PointNode* root;

	PointTree();

	void cluster();
};

