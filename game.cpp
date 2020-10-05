#include "precomp.h" // include (only) this in every .cpp file

Rasterizer rasterizer;
Camera camera;
vec3 position;

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::Init()
{
	// initialize rasterizer
	rasterizer.Init( screen );
	// setup camera (note: in ogl/glm, z for 'far' is -inf)
	position = vec3( 0, 0, 8 );
	camera.SetPosition( position );
	camera.LookAt( vec3( 0, 0, 0 ) );
	// initialize scene
	rasterizer.scene->Add( "assets/unity_full/unityScene.obj" );
	// load the camera, if possible
	FILE* f = fopen( "camera.dat", "rb" );
	if (!f) return;
	fread( &camera.transform, 1, sizeof( mat4 ), f );
	position = camera.GetPosition();
	fclose( f );
	// spline file
	FILE* sf = fopen( "spline.dat", "rb" );
	while (!feof( sf ))
	{
		vec3 p, t;
		fread( &p, 1, sizeof( vec3 ), sf );
		fread( &t, 1, sizeof( vec3 ), sf );
		sp[N] = p, st[N++] = t;
	}
	N--; // last one is garbage
}

// -----------------------------------------------------------
// Close down the app
// -----------------------------------------------------------
void Game::Shutdown()
{
	// store the camera to a file
	FILE* f = fopen( "camera.dat", "wb" );
	fwrite( &camera.transform, 1, sizeof( mat4 ), f );
	fclose( f );
}

// -----------------------------------------------------------
// Input handling
// -----------------------------------------------------------
void Game::HandleInput( float dt )
{
	if (GetAsyncKeyState( 'W' )) position += camera.GetForward() * dt * 0.4f;
	if (GetAsyncKeyState( 'S' )) position -= camera.GetForward() * dt * 0.4f;
	if (GetAsyncKeyState( 'A' )) position -= camera.GetRight() * dt * 0.4f;
	if (GetAsyncKeyState( 'D' )) position += camera.GetRight() * dt * 0.4f;
	if (GetAsyncKeyState( 'R' )) position += camera.GetUp() * dt * 0.4f;
	if (GetAsyncKeyState( 'F' )) position -= camera.GetUp() * dt * 0.4f;
	camera.SetPosition( position );
	if (GetAsyncKeyState( VK_LEFT )) camera.LookAt( camera.GetPosition() + camera.GetForward() - dt * 0.02f * camera.GetRight() );
	if (GetAsyncKeyState( VK_RIGHT )) camera.LookAt( camera.GetPosition() + camera.GetForward() + dt * 0.02f * camera.GetRight() );
	if (GetAsyncKeyState( VK_UP )) camera.LookAt( camera.GetPosition() + camera.GetForward() - dt * 0.02f * camera.GetUp() );
	if (GetAsyncKeyState( VK_DOWN )) camera.LookAt( camera.GetPosition() + camera.GetForward() + dt * 0.02f * camera.GetUp() );
}

// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
void Game::Tick( float deltaTime )
{
#if 0
	HandleInput( 1 );
#else
	// get four path vertices for position interpolation
	vec3 p0 = sp[(C + N - 1) % N];
	vec3 p1 = sp[C];
	vec3 p2 = sp[(C + 1) % N];
	vec3 p3 = sp[(C + 2) % N];
	// get four path vertices for target interpolation
	vec3 t0 = st[(C + N - 1) % N];
	vec3 t1 = st[C];
	vec3 t2 = st[(C + 1) % N];
	vec3 t3 = st[(C + 2) % N];
	static float t = 0; // ugly static, love it
	// Catmull-Rom spline interpolation
	vec3 pos = Spline( p0, p1, p2, p3, t );
	vec3 target = Spline( t0, t1, t2, t3, t );
	// pass to camera
	camera.SetPosition( pos );
	camera.LookAt( target );
	// update path time
	if ((t += 0.02f) >= 1) /* next segment */ t -= 1, C = (++C >= N ? 0 : C);
#endif
	screen->Clear( 0 );
	rasterizer.Render( camera );
}