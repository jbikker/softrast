#pragma once

namespace Tmpl8 {

// -----------------------------------------------------------
// Texture class
// encapsulates a palettized pixel surface with pre-scaled
// palettes for fast shading
// -----------------------------------------------------------
class Texture
{
public:
	// constructor / destructor
	Texture() : name( 0 ) {}
	Texture( char* file );
	~Texture();
	// methods
	void SetName( const char* name );
	void Load( const char* file );
	// data members
	char* name;
	Surface8* pixels;
};

// -----------------------------------------------------------
// Material class
// basic material properties
// -----------------------------------------------------------
class Material
{
public:
	// constructor / destructor
	Material() : texture( 0 ), name( 0 ) {}
	~Material();
	// methods
	void SetName( char* name );
	// data members
	uint diffuse;					// diffuse material color
	Texture* texture;				// texture
	char* name;						// material name
};

// -----------------------------------------------------------
// SGNode class
// scene graph node, with convenience functions for translate
// and transform; base class for Mesh
// -----------------------------------------------------------
class SGNode
{
public:
	enum
	{
		SG_TRANSFORM = 0,
		SG_MESH
	};
	// constructor / destructor
	~SGNode();
	// methods
	void SetPosition( vec3& pos ) { mat4& M = localTransform; M[3] = pos.x, M[7] = pos.y, M[11] = pos.z; }
	vec3 GetPosition() { mat4& M = localTransform; return vec3( M[3], M[7], M[11] ); }
	void RotateX( float x ) { RotateABC( x, 0, 0, 0, 1, 2 ); }
	void RotateY( float y ) { RotateABC( y, 0, 0, 1, 0, 2 ); }
	void RotateZ( float z ) { RotateABC( z, 0, 0, 2, 1, 0 ); }
	void RotateXYZ( float x, float y, float z ) { RotateABC( x, y, z, 0, 1, 2 ); }
	void RotateXZY( float x, float y, float z ) { RotateABC( x, z, y, 0, 2, 1 ); }
	void RotateYXZ( float x, float y, float z ) { RotateABC( y, x, z, 1, 0, 2 ); }
	void RotateZXY( float x, float y, float z ) { RotateABC( z, x, y, 2, 0, 1 ); }
	void RotateYZX( float x, float y, float z ) { RotateABC( y, z, x, 1, 2, 0 ); }
	void RotateZYX( float x, float y, float z ) { RotateABC( z, y, x, 2, 1, 0 ); }
	void Add( SGNode* node ) { child.push_back( node ); }
	void Render( mat4& transform );
	virtual int GetType() { return SG_TRANSFORM; }
private:
	void RotateABC( float a, float b, float c, int a1, int a2, int a3 );
	// data members
public:
	mat4 localTransform;
	vector<SGNode*> child;
};

// -----------------------------------------------------------
// Mesh class
// represents a mesh
// -----------------------------------------------------------
class Mesh : public SGNode
{
public:
	// constructor / destructor
	Mesh() : verts( 0 ), tris( 0 ), pos( 0 ), uv( 0 ), spos( 0 ) {}
	Mesh( int vcount, int tcount );
	~Mesh();
	// methods
	void Render( mat4& transform );
	virtual int GetType() { return SG_MESH; }
	// data members
	vec3* pos;						// object-space vertex positions
	vec3* tpos;						// world-space positions
	vec2* uv;						// vertex uv coordinates
	vec2* spos;						// screen positions
	vec3* norm;						// vertex normals
	vec3* N;						// triangle plane
	int* tri;						// connectivity data
	int verts, tris;				// vertex & triangle count
	Material* material;				// mesh material
	vec3 bounds[2];					// mesh bounds
	static Surface* screen;
	static float* xleft, *xright;	// outline tables for rasterization
	static float* uleft, *uright;
	static float* vleft, *vright;
	static float* zleft, *zright; 
};

// -----------------------------------------------------------
// Scene class
// owner of the scene graph;
// owner of the material and texture list
// -----------------------------------------------------------
class Scene
{
public:
	// constructor / destructor
	Scene() : root( 0 ), scenePath( 0 ) {}
	~Scene();
	// methods
	void Render();
	SGNode* Add( char* file, float scale = 1.0f ) { SGNode* n = LoadOBJ( file, scale ); root->Add( n ); return n; }
	SGNode* LoadOBJ( const char* file, const float scale );
	Material* FindMaterial( const char* name );
	Texture* FindTexture( const char* name );
private:
	void ExtractPath( const char* file );
	void LoadMTL( const char* file );
	// data members
public:
	SGNode* root;
	vector<Material*> matList;
	vector<Texture*> texList;
	char* scenePath;
};

// -----------------------------------------------------------
// Camera class
// convenience class for storing the camera transform
// -----------------------------------------------------------
class Camera
{
public:
	// methods
	void SetPosition( vec3& pos ) { mat4& M = transform; M[3] = pos.x, M[7] = pos.y, M[11] = pos.z; }
	vec3 GetPosition() { mat4& M = transform; return vec3( M[3], M[7], M[11] ); }
	vec3 GetRight() { mat4& M = transform; return vec3( M[0], M[4], M[8] ); }
	vec3 GetUp() { mat4& M = transform; return vec3( M[1], M[5], M[9] ); }
	vec3 GetForward() { mat4& M = transform; return -vec3( M[2], M[6], M[10] ); }
	void LookAt( vec3 target )
	{
		vec3 L = normalize( GetPosition() - target ), up = vec3( 0, 1, 0 );
		vec3 R = normalize( cross( L, up ) ), U = normalize( cross( R, L ) );
		mat4& M = transform;
		M[0] = R.x, M[4] = R.y, M[8] = R.z, 
		M[1] = U.x, M[5] = U.y, M[9] = U.z, 
		M[2] = L.x, M[6] = L.y, M[10] = L.z;
	}
	// data members
	mat4 transform;	
};

// -----------------------------------------------------------
// Rasterizer class
// rasterizer
// implements a basic, but fast & accurate software rasterizer,
// with the following features:
// - frustum culling (per mesh)
// - backface culling (per tri)
// - full frustum clipping, including near plane
// - perspective correct texture mapping
// - sub-pixel and sub-texel accuracy
// - z-buffering
// - basic shading (n dot l for an imaginary light source)
// - fast OBJ file loading with render state oriented mesh breakdown
// this rasterizer has been designed for educational purposes
// and is intentionally small and bare bones.
// -----------------------------------------------------------
class Rasterizer
{
public:
	// constructor / destructor
	Rasterizer() : scene( 0 ) {}
	~Rasterizer();
	// methods
	void Init( Surface* screen );
	void Render( Camera& camera );
	// data members
	Scene* scene;	
	static float* zbuffer;
	static vec4 frustum[5];
};

}; // namespace Tmpl8