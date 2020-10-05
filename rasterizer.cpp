#include "precomp.h"

// -----------------------------------------------------------
// static data for the rasterizer
// -----------------------------------------------------------
Surface* Mesh::screen = 0;
float* Mesh::xleft, *Mesh::xright, *Mesh::uleft, *Mesh::uright;
float* Mesh::vleft, *Mesh::vright, *Mesh::zleft, *Mesh::zright;
float* Rasterizer::zbuffer;
vec4 Rasterizer::frustum[5];
static vec3 raxis[3] = { vec3( 1, 0, 0  ), vec3( 0, 1, 0 ), vec3( 0, 0, 1 ) };

// -----------------------------------------------------------
// Tiny functions
// -----------------------------------------------------------
Texture::Texture( char* file ) : name( 0 ) { pixels = new Surface8( file ); SetName( file ); }
Texture::~Texture() { delete pixels; delete name; }
void Texture::SetName( const char* n ) { delete name; strcpy( name = new char[strlen( n ) + 1], n ); }
Material::~Material() { delete name; }
void Material::SetName( char* n ) { delete name; strcpy( name = new char[strlen( n ) + 1], n ); }
SGNode::~SGNode() { for( uint i = 0; i < child.size(); i++ ) delete child[i]; }
Rasterizer::~Rasterizer() { delete scene; }
Mesh::~Mesh() { delete pos; delete N; delete spos; delete tri; }

// -----------------------------------------------------------
// Mesh constructor
// input: vertex count & face count
// allocates room for mesh data: 
// - pos:  vertex positions
// - tpos: transformed vertex positions
// - norm: vertex normals
// - spos: vertex screen space positions
// - uv:   vertex uv coordinates
// - N:    face normals
// - tri:  connectivity data
// -----------------------------------------------------------
Mesh::Mesh( int vcount, int tcount ) : verts( vcount ), tris( tcount )
{
	pos = new vec3[vcount * 3], tpos = pos + vcount, norm = pos + 2 * vcount;
	spos = new vec2[vcount * 2], uv = spos + vcount, N = new vec3[tcount];
	tri = new int[tcount * 3];
}

// -----------------------------------------------------------
// Mesh render function
// input: final matrix for scene graph node
// renders a mesh using software rasterization.
// stages:
// 1. mesh culling: checks the mesh against the view frustum
// 2. vertex transform: calculates world space coordinates
// 3. triangle rendering loop. substages:
//    a) backface culling
//    b) clipping (Sutherland-Hodgeman)
//    c) shading (using pre-scaled palettes for speed)
//    d) projection: world-space to 2D screen-space
//    e) span construction
//    f) span filling
// -----------------------------------------------------------
void Mesh::Render( mat4& transform )
{
	// cull mesh
	vec3 c[8];
	for( int i = 0; i < 8; i++ ) c[i] = (transform * vec4( bounds[i & 1].x, bounds[(i >> 1) & 1].y, bounds[i >> 2].z, 1 )).xyz;
	for( int i, p = 0; p < 5; p++ ) 
	{
		for( i = 0; i < 8; i++ ) if ((dot( Rasterizer::frustum[p].xyz, c[i] ) - Rasterizer::frustum[p].w) > 0) break;
		if (i == 8) return;
	}
	// transform vertices
	for( int i = 0; i < verts; i++ ) tpos[i] = (transform * vec4( pos[i], 1 )).xyz;
	// draw triangles
	if (!material->texture) return; // for now: texture required.
	unsigned char* src = material->texture->pixels->GetBuffer();
	float* zbuffer = Rasterizer::zbuffer, f;
	const float tw = (float)material->texture->pixels->GetWidth();
	const float th = (float)material->texture->pixels->GetHeight();
	const int umask = (int)tw - 1, vmask = (int)th - 1;
	for( int i = 0; i < tris; i++ )
	{
		// cull triangle
		vec3 Nt = (transform * vec4( N[i], 0 )).xyz;
		if (dot( tpos[tri[i * 3 + 0]], Nt ) > 0) continue;
		// clip
		vec3 cpos[2][8], *pos;
		vec2 cuv[2][8], *tuv;
		int nin = 3, nout = 0, from = 0, to = 1, miny = SCRHEIGHT - 1, maxy = 0, h;
		for( int v = 0; v < 3; v++ ) cpos[0][v] = tpos[tri[i * 3 + v]], cuv[0][v] = uv[tri[i * 3 + v]];
		for( int p = 0; p < 2; p++, from = 1 - from, to = 1 - to, nin = nout, nout = 0 ) for( int v = 0; v < nin; v++ )
		{
			const vec3 A = cpos[from][v], B = cpos[from][(v + 1) % nin];
			const vec2 Auv = cuv[from][v], Buv = cuv[from][(v + 1) % nin];
			const vec4 plane = Rasterizer::frustum[p];
			const float t1 = dot( plane.xyz, A ) - plane.w, t2 = dot( plane.xyz, B ) - plane.w;
			if ((t1 < 0) && (t2 >= 0))
				f = t1 / (t1 - t2),
				cuv[to][nout] = Auv + (Buv - Auv) * f, cpos[to][nout++] = A + f * (B - A),
				cuv[to][nout] = Buv, cpos[to][nout++] = B;
			else if ((t1 >= 0) && (t2 >= 0)) cuv[to][nout] = Buv, cpos[to][nout++] = B;
			else if ((t1 >= 0) && (t2 < 0))
				f = t1 / (t1 - t2),
				cuv[to][nout] = Auv + (Buv - Auv) * f, cpos[to][nout++] = A + f * (B - A);
		}
		if (nin == 0) continue;
		// shade
		Pixel* pal = material->texture->pixels->GetPalette( (int)(max( 0.0f, Nt.z ) * (PALETTE_LEVELS - 1) ) );
		// project
		pos = cpos[from], tuv = cuv[from];
		for( int v = 0; v < nin; v++ )
			pos[v].x = ((pos[v].x * SCRWIDTH) / -pos[v].z) + SCRWIDTH / 2,
			pos[v].y = ((pos[v].y * SCRWIDTH) / pos[v].z) + SCRHEIGHT / 2;
		// draw
		for( int j = 0; j < nin; j++ )
		{
			int vert0 = j, vert1 = (j + 1) % nin;
			if (pos[vert0].y > pos[vert1].y) h = vert0, vert0 = vert1, vert1 = h;
			const float y0 = pos[vert0].y, y1 = pos[vert1].y, rydiff = 1.0f / (y1 - y0);
			if ((y0 == y1) || (y0 >= SCRHEIGHT) || (y1 < 1)) continue;
			const int iy0 = max( 1, (int)y0 + 1 ), iy1 = min( SCRHEIGHT - 2, (int)y1 );
			float x0 = pos[vert0].x, dx = (pos[vert1].x - x0) * rydiff;
			float z0 = 1.0f / pos[vert0].z, z1 = 1.0f / pos[vert1].z, dz = (z1 - z0) * rydiff;
			float u0 = tuv[vert0].x * z0, du = (tuv[vert1].x * z1 - u0) * rydiff;
			float v0 = tuv[vert0].y * z0, dv = (tuv[vert1].y * z1 - v0) * rydiff;
			const float f = (float)iy0 - y0;
			x0 += dx * f, u0 += du * f, v0 += dv * f, z0 += dz * f;
			for( int y = iy0; y <= iy1; y++ )
			{
				if (x0 < xleft[y]) xleft[y] = x0, uleft[y] = u0, vleft[y] = v0, zleft[y] = z0;
				if (x0 > xright[y]) xright[y] = x0, uright[y] = u0, vright[y] = v0, zright[y] = z0;
				x0 += dx, u0 += du, v0 += dv, z0 += dz;
			}
			miny = min( miny, iy0 ), maxy = max( maxy, iy1 );
		}
		for( int y = miny; y <= maxy; xleft[y] = SCRWIDTH - 1, xright[y++] = 0 )
		{
			float x0 = xleft[y], x1 = xright[y], rxdiff = 1.0f / (x1 - x0);
			float u0 = uleft[y], du = (uright[y] - u0) * rxdiff;
			float v0 = vleft[y], dv = (vright[y] - v0) * rxdiff;
			float z0 = zleft[y], dz = (zright[y] - z0) * rxdiff;
			const int ix0 = (int)x0 + 1, ix1 = min( SCRWIDTH - 2, (int)x1 );
			const float f = (float)ix0 - x0;
			u0 += f * du, v0 += f * dv, z0 += f * dz;
			Pixel* dest = screen->GetBuffer() + y * screen->GetWidth();
			float* zbuf = zbuffer + y * SCRWIDTH;
			for( int x = ix0; x <= ix1; x++, u0 += du, v0 += dv, z0 += dz ) // plot span
			{
				if (z0 >= zbuf[x]) continue;
				const float z = 1.0f / z0;
				const int u = (int)(u0 * z * tw) & umask, v = (int)(v0 * z * th) & vmask;
				dest[x] = pal[src[u + v * (umask + 1)]], zbuf[x] = z0;
			}
		}
	}
}

// -----------------------------------------------------------
// Scene destructor
// -----------------------------------------------------------
Scene::~Scene()
{
	delete root;
	for( uint i = 0; i < texList.size(); i++ ) delete texList[i];
	for( uint i = 0; i < matList.size(); i++ ) delete matList[i];
	delete scenePath;
}

// -----------------------------------------------------------
// Scene::ExtractPath
// retrieves the path from a file name;
// used to construct the path to mesh textures
// -----------------------------------------------------------
void Scene::ExtractPath( const char* file )
{
	char tmp[2048], *lastSlash = tmp;
	strcpy( tmp, file );
	while (strstr( lastSlash, "/" )) lastSlash = strstr( lastSlash, "/" ) + 1;
	while (strstr( lastSlash, "\\" )) lastSlash = strstr( lastSlash, "\\" ) + 1;
	*lastSlash = 0;
	delete scenePath;
	scenePath = new char[strlen( tmp ) + 1];
	strcpy( scenePath, tmp );
}

// -----------------------------------------------------------
// Scene::FindMaterial
// get a material pointer by material name
// -----------------------------------------------------------
Material* Scene::FindMaterial( const char* name )
{
	for( uint s = matList.size(), i = 0; i < s; i++ ) if (matList[i]->name) 
		if (!strcmp( matList[i]->name, name )) return matList[i];
	return 0;
}

// -----------------------------------------------------------
// Scene::FindTexture
// get a texture pointer by texture name
// -----------------------------------------------------------
Texture* Scene::FindTexture( const char* name )
{
	for( uint s = texList.size(), i = 0; i < s; i++ ) if (texList[i]->name)
		if (!strcmp( texList[i]->name, name )) return texList[i];
	return 0;
}

// -----------------------------------------------------------
// Scene::LoadMTL
// loads a MTL file for an OBJ file
// -----------------------------------------------------------
void Scene::LoadMTL( const char* file )
{
	char fname[1024], line[1024], cmd[256], matName[128];
	strcpy( fname, scenePath );
	if (fname[0]) strcat( fname, "/" );
	strcat( fname, file );
	FILE* f = fopen( fname, "r" );
	if (!f) return;
	Material* current = 0;
	int firstIdx = matList.size();
	while (!feof( f ))
	{
		line[0] = 0;
		fgets( line, 1022, f );
		if (!line[0]) continue;
		if (line[strlen( line ) - 1] < 32) line[strlen( line ) - 1] = 0; // clean '10' at end
		sscanf( line, "%s", cmd );
		if (!_stricmp( cmd, "newmtl" ))
		{
			sscanf( line + strlen( cmd ), "%s", matName );
			matList.push_back( current = new Material() );
			current->SetName( matName );
		}
		if (_stricmp( cmd, "map_Kd" )) continue;
		char* tname = strstr( line, " " );
		if (!tname) continue; else tname++;
		strcpy( fname, scenePath );
		strcat( fname, "textures/" );
		strcat( fname, tname );
		Texture* texture = FindTexture( fname );
		if (!texture) texList.push_back( texture = new Texture( fname ) );
		current->texture = texture;
	}
	fclose( f );
}

// -----------------------------------------------------------
// Scene::LoadOBJ
// loads an OBJ file, returns a scene graph node
// a scene graph node is produced for each mesh in the obj
// file, and for each unique material in each mesh. the
// resulting scene graph is ready for rendering in a state-
// driven renderer (e.g. OGL).
// -----------------------------------------------------------
SGNode* Scene::LoadOBJ( const char* file, const float _Scale )
{
	struct UniqueVertex 
	{ 
		UniqueVertex( int v, int n = -1, int t = -1 ) : vertex( v ), normal( n ), uv( t ), next( -1 ), subid( 0 ) {}
		int vertex, normal, uv, next, subid, idx;
	};
	SGNode* root = new SGNode(), *group = root;
	Mesh* current = 0, *nextMesh = 0;
	FILE* f = fopen( file, "r" );
	ExtractPath( file );
	if (f) // obj file loader: converts indexed obj file into indexed multi-mesh
	{
		vector<vec3> vlist, nlist, vlist_, nlist_;
		vector<vec2> uvlist, uvlist_;
		vector<uint> index, index_;
		vector<UniqueVertex> unique;
		vlist.reserve( 100000 );
		nlist.reserve( 100000 );
		unique.reserve( 100000 );
		int subID = 1, formata;
		while (!feof( f ))
		{
			current = nextMesh, subID++, formata = -1;
			bool hasUV = false;
			char line[2048], tmp[2048];
			while (!feof( f ))
			{
				line[0] = 0;
				fgets( line, 1023, f );
				if (!_strnicmp( line, "mtllib", 6 )) 
				{ 
					sscanf( line + 7, "%s", tmp ); 
					LoadMTL( tmp ); 
					formata = -1, hasUV = false;
				}
				if (!_strnicmp( line, "usemtl", 6 )) 
				{
					// prepare new mesh
					sscanf( line + 7, "%s", tmp );
					nextMesh = new Mesh();
					nextMesh->material = FindMaterial( tmp );
					group->child.push_back( nextMesh );
					subID++;
					break;
				}
				if (line[0] == 'g') 
				{
					formata = -1;
					char* g = line + 2;
					while ((g[0]) && (g[strlen( g ) - 1] < 33)) g[strlen( g ) - 1] = 0;
					root->child.push_back( group = new SGNode() );
				}
				if (line[0] == 'v') if (line[1] == ' ' ) 
				{ 
					vec3 vertex; 
					sscanf( line + 2, "%f %f %f", &vertex.x, &vertex.y, &vertex.z ); 
					vlist.push_back( vertex * _Scale ); 
					unique.push_back( UniqueVertex( vlist.size() - 1 ) );
				}
				else if (line[1] == 't' ) { vec2 uv; sscanf( line + 3, "%f %f", &uv.x, &uv.y ); uv.y = 1.0f - uv.y; uvlist.push_back( uv ); }
				else if (line[1] == 'n' ) { vec3 normal; sscanf( line + 3, "%f %f %f", &normal.x, &normal.y, &normal.z ); nlist.push_back( normal ); }
				if (line[0] != 'f') continue;
				if (formata == -1) 
				{
					formata = 0;
					for( int i = 0; i < (int)strlen( line ); i++ ) if (line[i] == '/' && line[i + 1] == '/') formata = 1;
				}
				int v[3], n[3], t[3] = { 0, 0, 0 };
				if (formata) sscanf( line + 2, "%i//%i %i//%i %i//%i", &v[0], &n[0], &v[1], &n[1], &v[2], &n[2] );
							 sscanf( line + 2, "%i/%i/%i %i/%i/%i %i/%i/%i", &v[0], &t[0], &n[0], &v[1], &t[1], &n[1], &v[2], &t[2], &n[2] );
				for ( int i = 0; i < 3; i++ )
				{
					int vidx = v[i] - 1, idx = vidx, lastIdx = idx, nidx = n[i] - 1, uvidx = t[i] - 1;
					if (uvidx > -1) hasUV = true;
					do
					{
						UniqueVertex& u = unique[idx];
						if (u.subid != subID) // vertex not used before by this mesh
						{
							u.subid = subID, u.next = -1, u.vertex = vidx, u.normal = nidx, u.uv = uvidx;
							index.push_back( idx );
							break;
						}
						else if ((u.normal == nidx) && (u.uv == uvidx)) // vertex used before, but the same
						{
							index.push_back( idx );
							break;
						}
						lastIdx = idx, idx = u.next;
					} while (idx > -1);
					if (idx != -1) continue;
					uint newIdx = unique.size();
					unique[lastIdx].next = newIdx;
					index.push_back( newIdx );
					unique.push_back( UniqueVertex( vidx, nidx, uvidx ) );
					unique[newIdx].subid = subID;
				}
			}
			if (!current) continue; else subID++;
			vlist_.clear();
			nlist_.clear();
			uvlist_.clear();
			index_.clear();
			for( uint i = 0; i < index.size(); i++ )
			{
				UniqueVertex& u = unique[index[i]];
				if (u.subid == subID) index_.push_back( u.idx ); else // first time we encounter this UniqueVertex, emit
				{
					vlist_.push_back( vlist[u.vertex] );
					nlist_.push_back( nlist[u.normal] );
					if (hasUV) uvlist_.push_back( uvlist[u.uv] );
					index_.push_back( vlist_.size() - 1 );
					u.idx = vlist_.size() - 1, u.subid = subID;
				}
			}
			// create mesh
			int nv = current->verts = vlist_.size(), nt = current->tris = index_.size() / 3;
			current->pos = new vec3[nv * 3], current->tpos = current->pos + nv;
			current->norm = current->pos + 2 * nv, current->spos = new vec2[nv * 2];
			current->N = new vec3[nt], current->uv = current->spos + nv;
			current->tri = new int[nt * 3];
			memcpy( current->pos, (vec3*)&vlist_[0], current->verts * sizeof( vec3 ) );
			memcpy( current->uv, (vec2*)&uvlist_[0], current->verts * sizeof( vec2 ) );
			memcpy( current->tri, (int*)&index_[0], current->tris * 3 * sizeof( int ) );
			memcpy( current->norm, (vec3*)&nlist_[0], current->verts * sizeof( vec3 ) );
			// calculate triangle planes
			for( int i = 0; i < current->tris; i++ )
			{
				vec3 v0 = vlist_[index_[i * 3 + 0]], v1 = vlist_[index_[i * 3 + 1]], v2 = vlist_[index_[i * 3 + 2]];
				current->N[i] = normalize( cross( v1 - v0, v2 - v0 ) );
				if (dot( current->N[i], nlist_[index_[i * 3 + 1]] ) < 0) current->N[i] *= -1.0f;
			}
			// calculate mesh bounds
			vec3& bmin = current->bounds[0], &bmax = current->bounds[1];
			bmin = { 1e30f, 1e30f, 1e30f }, bmax = { -1e30f, -1e30f, -1e30f };
			for( int i = 0; i < current->verts; i++ )
				bmin.x = min( bmin.x, current->pos[i].x ), bmax.x = max( bmax.x, current->pos[i].x ),
				bmin.y = min( bmin.y, current->pos[i].y ), bmax.y = max( bmax.y, current->pos[i].y ),
				bmin.z = min( bmin.z, current->pos[i].z ), bmax.z = max( bmax.z, current->pos[i].z );
			// clean up
			while (unique.size() > vlist.size()) unique.pop_back();
			index.clear();
		}
		fclose( f );
	}
	return root;
}

// -----------------------------------------------------------
// SGNode::Render
// recursive rendering of a scene graph node and its child nodes
// input: (inverse) camera transform
// -----------------------------------------------------------
void SGNode::Render( mat4& transform )
{
	mat4 M = transform * localTransform;
	if (GetType() == SG_MESH) ((Mesh*)this)->Render( M );
	for( uint i = 0; i < child.size(); i++ ) child[i]->Render( M );
}

// -----------------------------------------------------------
// SGNode::RotateABC
// helper function for the RotateXYZ permutations
// -----------------------------------------------------------
void SGNode::RotateABC( float a, float b, float c, int a1, int a2, int a3 )
{
	mat4 M = mat4::rotate( raxis[a1], a ) * mat4::rotate( raxis[a2], b ) * mat4::rotate( raxis[a3], c );
	for( int x = 0; x < 3; x++ ) for( int y = 0; y < 3; y++ ) localTransform[x + y * 4] = M[x + y * 4];
}

// -----------------------------------------------------------
// Rasterizer::Init
// initialization of the rasterizer
// input: surface to draw to
// -----------------------------------------------------------
void Rasterizer::Init( Surface* screen )
{
	// setup outline tables & zbuffer
	Mesh::xleft = new float[SCRHEIGHT], Mesh::xright = new float[SCRHEIGHT];
	Mesh::uleft = new float[SCRHEIGHT], Mesh::uright = new float[SCRHEIGHT];
	Mesh::vleft = new float[SCRHEIGHT], Mesh::vright = new float[SCRHEIGHT];
	Mesh::zleft = new float[SCRHEIGHT], Mesh::zright = new float[SCRHEIGHT];
	for( int y = 0; y < SCRHEIGHT; y++ ) Mesh::xleft[y] = SCRWIDTH - 1, Mesh::xright[y] = 0;
	zbuffer = new float[SCRWIDTH * SCRHEIGHT];
	// calculate view frustum planes
	float C = -1.0f, x1 = 0.5f, x2 = SCRWIDTH - 1.5f, y1 = 0.5f, y2 = SCRHEIGHT - 1.5f;
	vec3 p0 = { 0, 0, 0 };
	vec3 p1 = { ((x1 - SCRWIDTH / 2) * C) / SCRWIDTH, ((y1 - SCRHEIGHT / 2) * C) / SCRWIDTH, 1.0f };
	vec3 p2 = { ((x2 - SCRWIDTH / 2) * C) / SCRWIDTH, ((y1 - SCRHEIGHT / 2) * C) / SCRWIDTH, 1.0f };
	vec3 p3 = { ((x2 - SCRWIDTH / 2) * C) / SCRWIDTH, ((y2 - SCRHEIGHT / 2) * C) / SCRWIDTH, 1.0f };
	vec3 p4 = { ((x1 - SCRWIDTH / 2) * C) / SCRWIDTH, ((y2 - SCRHEIGHT / 2) * C) / SCRWIDTH, 1.0f };
	frustum[0] = { 0, 0, -1, 0.2f };
	frustum[1] = vec4( normalize( cross( p1 - p0, p4 - p1 ) ), 0 ); // left plane
	frustum[2] = vec4( normalize( cross( p2 - p0, p1 - p2 ) ), 0 ); // top plane
	frustum[3] = vec4( normalize( cross( p3 - p0, p2 - p3 ) ), 0 ); // right plane
	frustum[4] = vec4( normalize( cross( p4 - p0, p3 - p4 ) ), 0 ); // bottom plane
	// store screen pointer
	Mesh::screen = screen;
	// initialize scene
	(scene = new Scene())->root = new SGNode();
}

// -----------------------------------------------------------
// Rasterizer::Render
// render the scene
// input: camera to render with
// -----------------------------------------------------------
void Rasterizer::Render( Camera& camera )
{
	memset( zbuffer, 0, SCRWIDTH * SCRHEIGHT * sizeof( float ) );
	scene->root->Render( inverse( camera.transform ) );
}