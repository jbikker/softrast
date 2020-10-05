#pragma once

namespace Tmpl8 {

// -----------------------------------------------------------
// Raytracer struct
// generic ray
// -----------------------------------------------------------
class Ray
{
public:
	// constructor / destructor
	Ray( vec3 origin, vec3 direction, float distance ) : O( origin ), D( direction ), t( distance ) {}
	// methods
	void Intersect( Mesh* mesh, int triIdx );
	// data members
	vec3 O, D;
	float t;
};

// -----------------------------------------------------------
// Raytracer class
// to be build
// -----------------------------------------------------------
class Raytracer
{
public:
	// constructor / destructor
	Raytracer() : scene( 0 ) {}
	~Raytracer();
	// methods
	void Init( Surface* screen );
	void FindNearest( Ray& ray );
	bool IsOccluded( Ray& ray );
	void Render( Camera& camera );
	// data members
	Scene* scene;	
};

}; // namespace Tmpl8