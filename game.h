#pragma once

namespace Tmpl8
{

class Game
{
public:
	void SetTarget( Surface* surface ) { screen = surface; }
	void Init();
	void Shutdown();
	void Tick( float deltaTime );
	void HandleInput( float dt );
	void MouseUp( int button ) { /* implement if you want to detect mouse button presses */ }
	void MouseDown( int button ) { /* implement if you want to detect mouse button presses */ }
	void MouseMove( int x, int y ) { /* implement if you want to detect mouse movement */ }
	void KeyUp( int key ) { /* implement if you want to handle keys */ }
	void KeyDown( int key ) { /* implement if you want to handle keys */ }
	vec3 Spline( vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t )
	{
		float t01 = sqrtf( (p1 - p0).length() );
		float t12 = sqrtf( (p2 - p1).length() );
		float t23 = sqrtf( (p3 - p2).length() );
		vec3 m1 = p2 - p1 + t12 * ((p1 - p0) * (1 / t01) - (p2 - p0) * (1 / (t01 + t12)));
		vec3 m2 = p2 - p1 + t12 * ((p3 - p2) * (1 / t23) - (p3 - p1) * (1 / (t12 + t23)));
		vec3 a = 2.0f * (p1 - p2) + m1 + m2;
		vec3 b = -3.0f * (p1 - p2) - m1 - m1 - m2;
		return a * t * t * t + b * t * t + m1 * t + p1;
	}
private:
	Surface* screen;
	// spline path data
	vec3 sp[64], st[64];
	int N = 0, C = 0;
};

}; // namespace Tmpl8