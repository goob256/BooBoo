// DirectX .x model loader

#include "shim4/error.h"
#include "shim4/gfx.h"
#include "shim4/image.h"
#include "shim4/model.h"
#include "shim4/shader.h"
#include "shim4/shim.h"
#include "shim4/util.h"
#include "shim4/vertex_cache.h"

#include "shim4/internal/gfx.h"

using namespace noo;

namespace noo {

namespace gfx {

std::map<std::string, Model::Instance *> Model::loaded_models;

void Model::static_start()
{
	loaded_models.clear();
}

void Model::update_all()
{
	Uint32 elapsed = 1000 / shim::logic_rate;

	std::map<std::string, Instance *>::iterator it;
	for (it = loaded_models.begin(); it != loaded_models.end(); it++) {
		const std::pair<std::string, Instance *> &p = *it;
		Instance *instance = p.second;
		if (instance->started) {
			instance->elapsed += elapsed;
			const std::pair<std::string, Bone *> &p2 = *instance->animations[instance->current_animation]->bones.begin();
			int num_frames = (int)p2.second->frames.size();
			int ms_per_frame = 1000 / instance->frames_per_second;
			if (instance->finished_callback != 0 && (int)instance->elapsed >= num_frames * ms_per_frame) {
				instance->elapsed = instance->elapsed % (num_frames * ms_per_frame);
				// Back up so you can chain these
				util::Callback bak_callback = instance->finished_callback;
				void *bak_data = instance->finished_callback_data;
				instance->finished_callback = 0;
				// Don't need to unset data, though it could change
				bak_callback(bak_data);
			}
		}
	}
}

Model::Weights::Weights() :
	weights(0)
{
}

Model::Weights::~Weights()
{
	delete[] weights;
}

Model::Node *Model::Node::find(std::string name)
{
	if (this->name == name) {
		return this;
	}
	for (size_t i = 0; i < children.size(); i++) {
		Node *m = children[i]->find(name);
		if (m) {
			return m;
		}
	}

	return 0;
}

void Model::Node::animate(Animation *animation, int frame, glm::mat4 *transform)
{
	std::map<std::string, Bone *>::iterator it = animation->bones.find(name);
	if (it != animation->bones.end()) {
		const std::pair<std::string, Bone *> &p = *it;
		Bone *b = p.second;
		glm::mat4 this_trans = b->frames[frame];
		b->combined_transform = *transform * this_trans;
		for (size_t i = 0; i < children.size(); i++) {
			children[i]->animate(animation, frame, &b->combined_transform);
		}
	}
	else {
		for (size_t i = 0; i < children.size(); i++) {
			children[i]->animate(animation, frame, transform);
		}
	}
}

Model::Node::Node() :
	vertices(0),
	animated_vertices(0),
	face_textures(0),
	influences(0),
	min_x(FLT_MAX),
	min_y(FLT_MAX),
	min_z(FLT_MAX),
	max_x(-FLT_MAX),
	max_y(-FLT_MAX),
	max_z(-FLT_MAX)
{
}

Model::Node::~Node()
{
	delete[] vertices;
	delete[] animated_vertices;
	delete[] face_textures;
	delete[] influences;

	for (size_t i = 0; i < weights.size(); i++) {
		delete weights[i];
	}
	
	for (size_t i = 0; i < textures.size(); i++) {
		delete textures[i];
	}
}

#ifdef _WIN32
void *Model::Node::operator new(size_t i)
{
	return _mm_malloc(i,16);
}

void Model::Node::operator delete(void* p)
{
	_mm_free(p);
}
#endif

//--

#ifdef _WIN32
void *Model::Weights::operator new(size_t i)
{
	return _mm_malloc(i,16);
}

void Model::Weights::operator delete(void* p)
{
	_mm_free(p);
}
#endif

//--

#ifdef _WIN32
void *Model::Bone::operator new(size_t i)
{
	return _mm_malloc(i,16);
}

void Model::Bone::operator delete(void* p)
{
	_mm_free(p);
}
#endif

//--

Model::Model()
{
	instance = new Instance;
	instance->finished_callback = 0;
	instance->started = false;
	instance->elapsed = 0;
	instance->frames_per_second = 60;
}

Model::Model(std::string filename, bool load_from_filesystem) :
	line(1)
{
	instance = new Instance;
	instance->finished_callback = 0;
	instance->started = false;
	instance->elapsed = 0;
	instance->frames_per_second = 60;

	read(filename, load_from_filesystem);
}

Model::~Model()
{
	for (std::map<std::string, float **>::iterator it = precalculated.begin(); it != precalculated.end(); it++) {
		std::pair<std::string, float **> p = *it;
		float **f = p.second;
		Animation *anim = instance->animations[p.first];
		std::pair<std::string, Bone *> p2 = *(anim->bones.begin());
		Bone *bone = p2.second;
		size_t num_frames = bone->frames.size() * ((float)anim->precalc_fps/instance->frames_per_second);
		for (size_t i = 0; i < num_frames; i++) {
			delete[] f[i];
		}
		delete[] f;
	}

	for (size_t i = 0; i < roots.size(); i++) {
		destroy(roots[i]);
	}

	std::map<std::string, Animation *>::iterator it;
	for (it = instance->animations.begin(); it != instance->animations.end(); it++) {
		const std::pair<std::string, Animation *> &p = *it;
		destroy(p.second);
	}

	delete instance;
}

void Model::destroy(Node *node)
{
	for (size_t i = 0; i < node->children.size(); i++) {
		destroy(node->children[i]);
	}
	delete node;
}

void Model::destroy(Animation *animation)
{
	std::map<std::string, Bone *>::iterator it;
	for (it = animation->bones.begin(); it != animation->bones.end(); it++) {
		const std::pair<std::string, Bone *> &p = *it;
		delete p.second;
	}
	delete animation;
}

void Model::read(std::string filename, bool load_from_filesystem)
{
	SDL_RWops *file;

	if (load_from_filesystem) {
		file = SDL_RWFromFile(filename.c_str(), "rb");
	}
	else {
		filename = "gfx/models/" + filename;
		int sz;
		file = util::open_file(filename, &sz);
	}
	
	if (file == 0) {
		throw util::FileNotFoundError("Model " + filename + " not found!");
	}

	char buf[5];
	buf[4] = 0;
	
	if (SDL_RWread(file, buf, 1, 4) != 4) {
		throw util::LoadError("Error reading model header!");
	}

	if (std::string(buf) == "xof ") {
		char header[16];

		memcpy(header, buf, 4);

		if (SDL_RWread(file, header+4, 1, 12) != 12) {
			throw util::LoadError("Error reading model header!");
		}

		char buf[5];
		buf[4] = 0;

		memcpy(buf, header+8, 4);

		if (std::string(buf) == "txt ") {
			read_text_model(file);
		}
		else {
			throw util::LoadError("Binary .X not supported!");
		}
	}
	else if (std::string(buf) == "nsm ") {
		read_binary_model(file);
	}
	else {
		throw util::LoadError("This model type cannot be loaded by this program");
	}

	util::close_file(file);

	loaded_models[filename] = instance;
}

std::vector<Model::Node *> Model::get_nodes()
{
	return roots;
}

Model::Node *Model::find(std::string name)
{
	for (size_t i = 0; i < roots.size(); i++) {
		Node *m = roots[i]->find(name);
		if (m != 0) {
			return m;
		}
	}

	return 0;
}

int Model::read_byte(SDL_RWops *file)
{
	if (ungot.size() > 0) {
		int ret = ungot[ungot.size()-1];
		ungot.pop_back();
		return ret;
	}

	int c = util::SDL_fgetc(file);
	if (c == '\n') {
		line++;
	}
	return c;
}

void Model::unget(int c)
{
	ungot.push_back(c);
}

void Model::skip_whitespace(SDL_RWops *file)
{
	while (true) {
		int c = read_byte(file);
		if (c == EOF) {
			return;
		}
		else if (c == '/') {
			c = read_byte(file);
			if (c == '/') {
				while (true) {
					c = read_byte(file);
					if (c == EOF || c == '\n') {
						break;
					}
				}
			}
			else {
				unget(c);
				unget('/');
				return;
			}
		}
		else if (!isspace(c)) {
			unget(c);
			return;
		}
	}

	assert(0 && "Error skipping whitespace");
}

std::string Model::read_word(SDL_RWops *file)
{
	std::string word;

	while (true) {
		int c = read_byte(file);
		if (isspace(c) || c == EOF || c == ',' || c == ';') {
			// skip extra semicolons and commas
			while (true) {
				c = read_byte(file);
				if (c == ';' || c == ',') {
					continue;
				}
				unget(c);
				break;
			}
			return word;
		}
		char s[2];
		s[0] = c;
		s[1] = 0;
		word += s;
	}

	assert(0 && "Error reading word");
}

Model::Node *Model::read_text_frame(SDL_RWops *file)
{
	Node *m = new Node;

	float *vertices = 0;
	int *triangles = 0;
	float *normals = 0;
	int *normal_indices = 0;
	float *texcoords = 0;
	float *colours = 0;
	int num_triangles = 0;

	skip_whitespace(file);

	m->name = read_word(file);

	if (m->name == "") {
		destroy(m);
		throw util::LoadError(util::string_printf("Expected frame name on line %d", line));
	}

	skip_whitespace(file);

	int c = read_byte(file);

	if (c != '{') {
		destroy(m);
		throw util::LoadError(util::string_printf("Expected { on line %d", line));
	}

	while (true) {
		skip_whitespace(file);

		std::string token = read_word(file);

		if (token == "") {
			destroy(m);
			throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
		}
		else if (token == "}") {
			break;
		}
		else if (token == "Frame") {
			Node *child;
			try {
				child = read_text_frame(file);
			}
			catch (util::Error &) {
				destroy(m);
				throw;
			}
			child->parent = m;
			m->children.push_back(child);
		}
		else if (token == "FrameTransformMatrix") {
			skip_whitespace(file);
			c = read_byte(file);
			if (c != '{') {
				destroy(m);
				throw util::LoadError(util::string_printf("Expected { on line %d", line));
			}
			float f[16];
			for (int i = 0; i < 16; i++) {
				skip_whitespace(file);
				std::string word = read_word(file);
				if (word == "") {
					destroy(m);
					throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
				}
				f[i] = (float)atof(word.c_str());
			}
			skip_whitespace(file);
			c = read_byte(file);
			if (c != '}') {
				destroy(m);
				throw util::LoadError(util::string_printf("Expected } on line %d", line));
			}
			m->transform = glm::mat4(
				f[0],  f[1],  f[2],  f[3],
				f[4],  f[5],  f[6],  f[7],
				f[8],  f[9],  f[10], f[11],
				f[12], f[13], f[14], f[15]
			);
		}
		else if (token == "Mesh") {
			skip_whitespace(file);
			c = read_byte(file);
			if (c != '{') {
				destroy(m);
				throw util::LoadError(util::string_printf("Expected { on line %d", line));
			}
			skip_whitespace(file);
			std::string vcount_s = read_word(file);
			if (vcount_s == "") {
				destroy(m);
				throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
			}
			int vcount = atoi(vcount_s.c_str());
			vertices = new float[vcount * 3];
			for (int i = 0; i < vcount * 3; i++) {
				skip_whitespace(file);

				std::string v_s = read_word(file);

				if (v_s == "") {
					destroy(m);
					throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
				}

				float v = (float)atof(v_s.c_str());

				int comp = i % 3; // component: x=0, y=1, z=2

				if (comp == 0) { // x
					if (v < m->min_x) {
						m->min_x = v;
					}
					if (v > m->max_x) {
						m->max_x = v;
					}
				}
				else if (comp == 1) { // y
					if (v < m->min_y) {
						m->min_y = v;
					}
					if (v > m->max_y) {
						m->max_y = v;
					}
				}
				else { // z
					if (v < m->min_z) {
						m->min_z = v;
					}
					if (v > m->max_z) {
						m->max_z = v;
					}
				}
				
				vertices[i] = v;
			}
			skip_whitespace(file);
			std::string nfaces_s = read_word(file);
			if (nfaces_s == "") {
				destroy(m);
				throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
			}
			int nfaces = atoi(nfaces_s.c_str());
			num_triangles = nfaces;
			triangles = new int[nfaces * 3];
			for (int i = 0; i < nfaces * 4; i++) {
				skip_whitespace(file);
				std::string v_s = read_word(file);
				if (v_s == "") {
					destroy(m);
					throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
				}
				int v = atoi(v_s.c_str());
				if (i % 4 == 0 && v != 3) {
					destroy(m);
					throw util::LoadError("Only triangles supported!");
				}
				else if (i % 4 != 0) {
					int face = i / 4;
					int vert = i % 4 - 1;
					triangles[face*3+(2-vert)] = v;
					//triangles[face*3+vert] = v;
				}
			}
			while (true) {
				skip_whitespace(file);
				std::string token = read_word(file);
				if (token == "") {
					destroy(m);
					throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
				}
				else if (token == "}") {
					break;
				}
				if (token == "MeshNormals") {
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '{') {
						destroy(m);
						throw util::LoadError(util::string_printf("Expected { on line %d", line));
					}
					skip_whitespace(file);
					std::string nnormals_s = read_word(file);
					if (nnormals_s == "") {
						destroy(m);
						throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
					}
					int nnormals = atoi(nnormals_s.c_str());
					normals = new float[nnormals * 3];
					for (int i = 0; i < nnormals * 3; i++) {
						skip_whitespace(file);
						std::string v_s = read_word(file);
						if (v_s == "") {
							destroy(m);
							throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
						}
						float v = (float)atof(v_s.c_str());
						normals[i] = v;
					}
					skip_whitespace(file);
					std::string nfaces_s = read_word(file);
					if (nfaces_s == "") {
						destroy(m);
						throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
					}
					int nfaces = atoi(nfaces_s.c_str());
					normal_indices = new int[nfaces * 3];
					for (int i = 0; i < nfaces; i++) {
						skip_whitespace(file);
						std::string v_s = read_word(file);
						if (v_s == "") {
							destroy(m);
							throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
						}
						int count = atoi(v_s.c_str());
						if (count != 3) {
							destroy(m);
							throw util::LoadError("Only triangles supported, aborting!");
						}

						skip_whitespace(file);
						std::string v1 = read_word(file);
						skip_whitespace(file);
						std::string v2 = read_word(file);
						skip_whitespace(file);
						std::string v3 = read_word(file);

						int v1i = atoi(v1.c_str());
						int v2i = atoi(v2.c_str());
						int v3i = atoi(v3.c_str());

						normal_indices[i*3+2] = v1i;
						normal_indices[i*3+1] = v2i;
						normal_indices[i*3+0] = v3i;
					}
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '}') {
						destroy(m);
						throw util::LoadError(util::string_printf("Expected } on line %d", line));
					}
				}
				else if (token == "XSkinMeshHeader") {
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '{') {
						destroy(m);
						throw util::LoadError(util::string_printf("Expected } on line %d", line));
					}
					for (int i = 0; i < 3; i++) {
						// FIXME: read properly
						skip_whitespace(file);
						read_word(file);
					}
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '}') {
						destroy(m);
						throw util::LoadError(util::string_printf("Expected } on line %d", line));
					}
				}
				else if (token == "SkinWeights") {
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '{') {
						destroy(m);
						throw util::LoadError(util::string_printf("Expected } on line %d", line));
					}
					skip_whitespace(file);
					Weights *w = new Weights;
					w->name = read_word(file);
					if (w->name == "") {
						destroy(m);
						throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
					}
					w->name = w->name.substr(1, w->name.length()-2); // remove quotes
					skip_whitespace(file);
					std::string count_s = read_word(file);
					if (count_s == "") {
						destroy(m);
						throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
					}
					int count = atoi(count_s.c_str());
					std::vector<int> indices;
					std::vector<float> weights;
					for (int i = 0; i < count; i++) {
						skip_whitespace(file);
						std::string index_s = read_word(file);
						if (index_s == "") {
							destroy(m);
							throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
						}
						indices.push_back(atoi(index_s.c_str()));
					}
					for (int i = 0; i < count; i++) {
						skip_whitespace(file);
						std::string weight_s = read_word(file);
						if (weight_s == "") {
							destroy(m);
							delete w;
							throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
						}
						weights.push_back((float)atof(weight_s.c_str()));
					}
					w->weights = new float[vcount];
					for (int i = 0; i < vcount; i++) {
						w->weights[i] = 0.0f;
					}
					for (size_t i = 0; i < indices.size(); i++) {
						w->weights[indices[i]] = weights[i];
					}
					float f[16];
					for (int i = 0; i < 16; i++) {
						skip_whitespace(file);
						std::string v_s = read_word(file);
						if (v_s == "") {
							destroy(m);
							delete w;
							throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
						}
						f[i] = (float)atof(v_s.c_str());
					}
					w->transform = glm::mat4x4(
						f[0],  f[1],  f[2],  f[3],
						f[4],  f[5],  f[6],  f[7],
						f[8],  f[9],  f[10], f[11],
						f[12], f[13], f[14], f[15]
					);
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '}') {
						destroy(m);
						delete w;
						throw util::LoadError(util::string_printf("Expected } on line %d", line));
					}
					m->weights.push_back(w);
				}
				else if (token == "MeshTextureCoords") {
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '{') {
						destroy(m);
						throw util::LoadError(util::string_printf("Expected { on line %d", line));
					}
					skip_whitespace(file);
					std::string vcount_s = read_word(file);
					if (vcount_s == "") {
						destroy(m);
						throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
					}
					int vcount = atoi(vcount_s.c_str());
					texcoords = new float[vcount * 2];
					for (int i = 0; i < vcount * 2; i++) {
						skip_whitespace(file);
						std::string v_s = read_word(file);
						if (v_s == "") {
							destroy(m);
							throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
						}
						float v = (float)atof(v_s.c_str());
						if (i % 2 == 1) {
							texcoords[i] = 1.0f - v;
						}
						else {
							texcoords[i] = v;
						}
					}
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '}') {
						destroy(m);
						throw util::LoadError(util::string_printf("Expected } on line %d", line));
					}
				}
				else if (token == "MeshMaterialList") {
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '{') {
						destroy(m);
						throw util::LoadError(util::string_printf("Expected { on line %d", line));
					}
					skip_whitespace(file);
					std::string nmaterials_s = read_word(file);
					if (nmaterials_s == "") {
						destroy(m);
						throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
					}
					int nmaterials = atoi(nmaterials_s.c_str());
					skip_whitespace(file);
					std::string nfaces_s = read_word(file);
					if (nfaces_s == "") {
						destroy(m);
						throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
					}
					int nfaces = atoi(nfaces_s.c_str());
					m->face_textures = new int[nfaces];
					for (int i = 0; i < nfaces; i++) {
						skip_whitespace(file);
						std::string v_s = read_word(file);
						if (v_s == "") {
							destroy(m);
							throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
						}
						int v = atoi(v_s.c_str());
						m->face_textures[i] = v;
					}
					for (int i = 0; i < nmaterials; i++) {
						skip_whitespace(file);
						std::string token = read_word(file);
						if (token != "Material") {
							destroy(m);
							throw util::LoadError(util::string_printf("Expected Material on line %d", line));
						}
						skip_whitespace(file);
						token = read_word(file);
						if (token == "") {
							destroy(m);
							throw util::LoadError(util::string_printf("Unexpteced EOF on line %d", line));
						}

						skip_whitespace(file);
						c = read_byte(file);
						if (c != '{') {
							destroy(m);
							throw util::LoadError(util::string_printf("Expected { on line %d", line));
						}
						// we ignore these material properties for now
						for (int j = 0; j < 11; j++) {
							skip_whitespace(file);
							read_word(file);
						}
						skip_whitespace(file);
						token = read_word(file);
						if (token != "TextureFilename") {
							destroy(m);
							throw util::LoadError(util::string_printf("Expected TextureFilename on line %d", line));
						}
						int nquotes = 0;
						std::string filename;
						while (true) {
							c = read_byte(file);
							if (c == '"') {
								nquotes++;
							}
							else if (c == '}') {
								break;
							}
							else if (c == EOF) {
								destroy(m);
								throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
							}
							else if (nquotes == 1) {
								char s[2];
								s[0] = c;
								s[1] = 0;
								filename += s;
							}
						}
						Image *texture = new Image("gfx/textures/" + filename, true);
						if (texture) {
							m->textures.push_back(texture);
						}
						else {
							destroy(m);
							throw util::LoadError("Error loading texture 'gfx/textures/" + filename + "'");
						}
						skip_whitespace(file);
						c = read_byte(file);
						if (c != '}') {
							destroy(m);
							throw util::LoadError(util::string_printf("Expected } on line %d", line));
						}
					}
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '}') {
						destroy(m);
						throw util::LoadError(util::string_printf("Expected } on line %d", line));
					}
				}
				else if (token == "MeshVertexColors") {
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '{') {
						destroy(m);
						throw util::LoadError(util::string_printf("Expected { on line %d", line));
					}
					skip_whitespace(file);
					std::string ncolours_s = read_word(file);
					if (ncolours_s == "") {
						destroy(m);
						throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
					}
					int ncolours = atoi(ncolours_s.c_str());
					colours = new float[ncolours * 4];
					for (int i = 0; i < ncolours; i++) {
						skip_whitespace(file);
						std::string i_s = read_word(file);
						if (i_s == "") {
							destroy(m);
							throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
						}
						// FIXME: validate i_s (should be 4?)
						for (int j = 0; j < 4; j++) {
							skip_whitespace(file);
							std::string v_s = read_word(file);
							if (v_s == "") {
								destroy(m);
								throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
							}
							float v = atof(v_s.c_str());
							colours[i*4+j] = v;
						}
					}
					skip_whitespace(file);
					c = read_byte(file);
					if (c != '}') {
						destroy(m);
						throw util::LoadError(util::string_printf("Expected } on line %d", line));
					}
				}
				else {
					util::errormsg("Unexpected token %s on line %d.\n", token.c_str(), line);
					skip_section(file);
				}
			}
		}
		else {
			util::errormsg("Unexpected token %s on line %d.\n", token.c_str(), line);
			skip_section(file);
		}
	}

	m->create_arrays(vertices, triangles, normals, normal_indices, texcoords, colours, num_triangles);

	delete[] vertices;
	delete[] triangles;
	delete[] normals;
	delete[] normal_indices;
	delete[] texcoords;
	delete[] colours;

	return m;
}

void Model::Node::create_arrays(float *v, int *f, float *n, int *nind, float *t, float *c, int num_triangles)
{
	this->num_triangles = num_triangles;
	this->num_vertices = num_triangles * 3;

	vertices = new float[num_vertices * 12];
	animated_vertices = new float[num_vertices * 12];
	influences = new Influence[num_vertices];

	int vi = 0;

	if (textures.size() > 0) {
		for (size_t i = 0; i < textures.size(); i++) {
			for (int j = 0; j < num_triangles; j++) {
				int texture = face_textures[j];
				if (texture == (int)i) {
					for (int k = 0; k < 3; k++) {
						vertices[vi++] = v[f[j * 3 + k] * 3 + 0];
						vertices[vi++] = v[f[j * 3 + k] * 3 + 1];
						vertices[vi++] = v[f[j * 3 + k] * 3 + 2];
						if (n != 0) {
							vertices[vi++] = n[nind[j * 3 + k] * 3 + 0];
							vertices[vi++] = n[nind[j * 3 + k] * 3 + 1];
							vertices[vi++] = n[nind[j * 3 + k] * 3 + 2];
						}
						else {
							vi += 3;
						}
						if (t != 0) {
							vertices[vi++] = t[f[j * 3 + k] * 2 + 0];
							vertices[vi++] = t[f[j * 3 + k] * 2 + 1];
						}
						else {
							vi += 2;
						}
						if (c != 0) {
							vertices[vi++] = c[f[j * 3 + k] * 4 + 0];
							vertices[vi++] = c[f[j * 3 + k] * 4 + 1];
							vertices[vi++] = c[f[j * 3 + k] * 4 + 2];
							vertices[vi++] = c[f[j * 3 + k] * 4 + 3];
						}
						else {
							vertices[vi++] = 1.0f;
							vertices[vi++] = 1.0f;
							vertices[vi++] = 1.0f;
							vertices[vi++] = 1.0f;
						}
					}
				}
			}
		}
	}
	else {
		for (int j = 0; j < num_triangles; j++) {
			for (int k = 0; k < 3; k++) {
				vertices[vi++] = v[f[j * 3 + k] * 3 + 0];
				vertices[vi++] = v[f[j * 3 + k] * 3 + 1];
				vertices[vi++] = v[f[j * 3 + k] * 3 + 2];
				if (n != 0) {
					vertices[vi++] = n[nind[j * 3 + k] * 3 + 0];
					vertices[vi++] = n[nind[j * 3 + k] * 3 + 1];
					vertices[vi++] = n[nind[j * 3 + k] * 3 + 2];
				}
				else {
					vi += 3;
				}
				if (t != 0) {
					vertices[vi++] = t[f[j * 3 + k] * 2 + 0];
					vertices[vi++] = t[f[j * 3 + k] * 2 + 1];
				}
				else {
					vi += 2;
				}
				if (c != 0) {
					vertices[vi++] = c[f[j * 3 + k] * 4 + 0];
					vertices[vi++] = c[f[j * 3 + k] * 4 + 1];
					vertices[vi++] = c[f[j * 3 + k] * 4 + 2];
					vertices[vi++] = c[f[j * 3 + k] * 4 + 3];
				}
				else {
					vertices[vi++] = 1.0f;
					vertices[vi++] = 1.0f;
					vertices[vi++] = 1.0f;
					vertices[vi++] = 1.0f;
				}
			}
		}
	}

	memcpy(animated_vertices, vertices, sizeof(float)*num_vertices*12);

	for (int i = 0; i < num_vertices; i++) {
		influences[i] = Influence();
	}

	for (size_t i = 0; i < weights.size(); i++) {
		float *tmp = weights[i]->weights;
		weights[i]->weights = new float[num_vertices];
		int wi = 0;
		for (int j = 0; j < num_triangles; j++) {
			for (int k = 0; k < 3; k++) {
				weights[i]->weights[wi] = tmp[f[j * 3 + k]];
				if (weights[i]->weights[wi] != 0.0f) {
					influences[wi].weights.push_back(weights[i]);
				}
				wi++;
			}
		}
		delete[] tmp;
	}
}

Model::Animation *Model::read_animationset(SDL_RWops *file)
{
	Animation *a = new Animation;
	a->is_precalculated = false;

	skip_whitespace(file);

	a->name = read_word(file);

	if (a->name == "") {
		destroy(a);
		throw util::LoadError(util::string_printf("Expected AnimationSet name on line %d", line));
	}

	skip_whitespace(file);

	int c = read_byte(file);

	if (c != '{') {
		destroy(a);
		throw util::LoadError(util::string_printf("Expected { on line %d", line));
	}

	while (true) {
		skip_whitespace(file);

		std::string token = read_word(file);

		if (token == "") {
			destroy(a);
			throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
		}
		else if (token == "}") {
			break;
		}
		else if (token == "Animation") {
			Bone *bone = read_animation(file);
			if (bone == 0) {
				destroy(a);
				return 0;
			}
			a->bones[bone->name] = bone;
		}
		else {
			util::errormsg("Unexpected token %s on line %d.\n", token.c_str(), line);
			skip_section(file);
		}
	}

	return a;
}

Model::Bone *Model::read_animation(SDL_RWops *file)
{
	Bone *b = new Bone;

	skip_whitespace(file);

	int c = read_byte(file);

	if (c != '{') {
		delete b;
		throw util::LoadError(util::string_printf("Expected { on line %d", line));
	}

	skip_whitespace(file);

	std::string name = read_word(file);

	if (name[0] != '{' || name[name.length()-1] != '}') {
		delete b;
		throw util::LoadError(util::string_printf("Expected {bone_name} on line %d", line));
	}

	name = name.substr(1, name.length()-2);

	b->name = name;

	std::vector<glm::quat> rotations;
	std::vector<glm::vec3> scales;
	std::vector<glm::vec3> translations;

	while (true) {
		skip_whitespace(file);

		std::string token = read_word(file);

		if (token == "") {
			delete b;
			throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
		}
		else if (token == "}") {
			break;
		}
		else if (token == "AnimationKey") {
			skip_whitespace(file);

			int c = read_byte(file);

			if (c != '{') {
				delete b;
				throw util::LoadError(util::string_printf("Expected { on line %d", line));
			}

			skip_whitespace(file);

			std::string type = read_word(file);

			if (type != "0" && type != "1" && type != "2") {
				delete b;
				throw util::LoadError(util::string_printf("Unknown AnimationKey type %s on line %d", type.c_str(), line));
			}

			skip_whitespace(file);

			std::string countS = read_word(file);
			int count = atoi(countS.c_str());

			if (count <= 0) {
				delete b;
				throw util::LoadError(util::string_printf("Count is %d on line %d", count, line));
			}

			for (int i = 0; i < count; i++) {
				skip_whitespace(file);
				read_word(file); // skip frame number, we assume sequential
				skip_whitespace(file);
				std::string elem_countS = read_word(file);
				int elem_count = atoi(elem_countS.c_str());

				if (type == "0") { // rotation
					if (elem_count != 4) {
						delete b;
						throw util::LoadError(util::string_printf("Element count is not 4 on line %d", line));
					}
					float values[4];
					for (int j = 0; j < 4; j++) {
						skip_whitespace(file);
						std::string vS = read_word(file);
						if (vS == "") {
							delete b;
							throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
						}
						float v = atof(vS.c_str());
						values[j] = v;
					}
					glm::quat q;
					q.w = values[0];
					q.x = values[1];
					q.y = values[2];
					q.z = values[3];
					q = glm::inverse(q);
					rotations.push_back(q);
				}
				else {
					if (elem_count != 3) {
						delete b;
						throw util::LoadError(util::string_printf("Element count is not 3 on line %d", line));
					}
					float values[3];
					for (int j = 0; j < 3; j++) {
						skip_whitespace(file);
						std::string vS = read_word(file);
						if (vS == "") {
							delete b;
							throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
						}
						float v = atof(vS.c_str());
						values[j] = v;
					}
					glm::vec3 v;
					v.x = values[0];
					v.y = values[1];
					v.z = values[2];
					if (type == "1") {
						scales.push_back(v);
					}
					else {
						translations.push_back(v);
					}
				}
			}
			
			skip_whitespace(file);

			c = read_byte(file);

			if (c != '}') {
				delete b;
				throw util::LoadError(util::string_printf("Expected } on line %d", line));
			}
		}
		else {
			util::errormsg("Unexpected token %s on line %d.\n", token.c_str(), line);
			skip_section(file);
		}
	}

	if (rotations.size() != scales.size() || scales.size() != translations.size()) {
		delete b;
		throw util::LoadError("Unbalanced number of transformations!");
	}

	for (size_t i = 0; i < rotations.size(); i++) {
		glm::mat4 m;
		m = glm::translate(m, translations[i]);
		glm::mat4 m2 = glm::toMat4(rotations[i]);
		m = m * m2;
		m = glm::scale(m, scales[i]);
		b->frames.push_back(m);
	}

	return b;
}

void Model::skip_section(SDL_RWops *file)
{
	int open_count = 1;
	while (true) {
		int c = read_byte(file);
		if (c == '{') {
			break;
		}
	}
	while (true) {
		int c = read_byte(file);
		if (c == '{') open_count++;
		if (c == '}') open_count--;
		if (open_count == 0) break;
	}
}

void Model::read_text_model(SDL_RWops *file)
{
	while (true) {
		skip_whitespace(file);
		std::string token = read_word(file);
		if (token == "") {
			break;
		}
		else if (token == "template") {
			// FIXME:
			skip_section(file);
		}
		else if (token == "Frame") {
			Node *m = read_text_frame(file);
			m->parent = 0;
			roots.push_back(m);
		}
		else if (token == "AnimationSet") {
			Animation *a = read_animationset(file);
			instance->animations[a->name] = a;
		}
		else if (token == "AnimTicksPerSecond") {
			skip_whitespace(file);
			int c = read_byte(file);
			if (c != '{') {
				throw util::LoadError(util::string_printf("Expected { on line %d", line));
			}
			skip_whitespace(file);
			std::string word = read_word(file);
			if (word == "") {
				throw util::LoadError(util::string_printf("Unexpected EOF on line %d", line));
			}
			instance->frames_per_second = atoi(word.c_str());
			skip_whitespace(file);
			c = read_byte(file);
			if (c != '}') {
				throw util::LoadError(util::string_printf("Expected } on line %d", line));
			}
		}
	}
}

std::string Model::read_string(SDL_RWops *file)
{
	Uint32 len = SDL_ReadLE32(file);
	std::string str;
	char s[2];
	s[1] = 0;
	for (Uint32 i = 0; i < len; i++) {
		int c = util::SDL_fgetc(file);
		s[0] = c;
		str += std::string(s);
	}
	return str;
}

glm::mat4 Model::read_matrix(SDL_RWops *file)
{
	union {
		float f;
		Uint32 u;
	} u;

	glm::mat4 m;
	float *f = (float *)glm::value_ptr(m);

	for (int i = 0; i < 16; i++) {
		u.u = SDL_ReadLE32(file);
		f[i] = u.f;
	}

	return m;
}

Model::Node *Model::read_binary_frame(SDL_RWops *file)
{
	Node *n = new Node;
	
	n->min_x = FLT_MAX;
	n->min_y = FLT_MAX;
	n->min_z = FLT_MAX;
	n->max_x = -FLT_MAX;
	n->max_y = -FLT_MAX;
	n->max_z = -FLT_MAX;

	union {
		float f;
		Uint32 u;
	} u;

	n->name = read_string(file);

	Uint32 num_children = SDL_ReadLE32(file);

	for (size_t i = 0; i < num_children; i++) {
		Node *child = read_binary_frame(file);
		child->parent = n;
		n->children.push_back(child);
	}

	n->transform = read_matrix(file);

	n->num_vertices = SDL_ReadLE32(file);
	
	Uint32 num_floats = n->num_vertices * 12;

	n->vertices = new float[num_floats];

	for (Uint32 i = 0; i < num_floats; i++) {
		u.u = SDL_ReadLE32(file);
		n->vertices[i] = u.f;
		if (i % 12 == 0) { // x
			if (u.f < n->min_x) {
				n->min_x = u.f;
			}
			if (u.f > n->max_x) {
				n->max_x = u.f;
			}
		}
		if (i % 12 == 1) { // y
			if (u.f < n->min_y) {
				n->min_y = u.f;
			}
			if (u.f > n->max_y) {
				n->max_y = u.f;
			}
		}
		if (i % 12 == 2) { // z
			if (u.f < n->min_z) {
				n->min_z = u.f;
			}
			if (u.f > n->max_z) {
				n->max_z = u.f;
			}
		}
	}

	n->animated_vertices = new float[num_floats];
	memcpy(n->animated_vertices, n->vertices, sizeof(float)*num_floats);

	Uint32 num_textures = SDL_ReadLE32(file);

	for (Uint32 i = 0; i < num_textures; i++) {
		std::string image_filename = read_string(file);
		n->textures.push_back(new Image(image_filename, true));
	}

	n->num_triangles = SDL_ReadLE32(file);

	if (num_textures == 0) {
		n->face_textures = 0;
	}
	else {
		n->face_textures = new int[n->num_triangles];

		for (int i = 0; i < n->num_triangles; i++) {
			n->face_textures[i] = SDL_ReadLE32(file);
		}
	}

	Uint32 num_weights = SDL_ReadLE32(file);

	for (Uint32 i = 0; i < num_weights; i++) {
		Weights *w = new Weights;
		w->name = read_string(file);
		w->weights = new float[n->num_vertices];
		for (int j = 0; j < n->num_vertices; j++) {
			u.u = SDL_ReadLE32(file);
			w->weights[j] = u.f;
		}
		w->transform = read_matrix(file);
		n->weights.push_back(w);
	}

	std::vector<Bone *> bones;
	for (std::map<std::string, Animation *>::iterator it = instance->animations.begin(); it != instance->animations.end(); it++) {
		std::pair<std::string, Animation *> p = *it;
		Animation *a = p.second;
		for (std::map<std::string, Bone *>::iterator it2 = a->bones.begin(); it2 != a->bones.end(); it2++) {
			std::pair<std::string, Bone *> p2 = *it2;
			bones.push_back(p2.second);
		}
	}

	n->influences = new Influence[n->num_vertices];

	for (int i = 0; i < n->num_vertices; i++) {
		Uint32 num_weights = SDL_ReadLE32(file);
		for (size_t j = 0; j < num_weights; j++) {
			Uint32 index = SDL_ReadLE32(file);
			n->influences[i].weights.push_back(n->weights[index]);
		}
		Uint32 num_bones = SDL_ReadLE32(file);
		for (size_t j = 0; j < num_bones; j++) {
			Uint32 index = SDL_ReadLE32(file);
			n->influences[i].bones.push_back(bones[index]);
		}
	}

	return n;
}

Model::Animation *Model::read_binary_animation(SDL_RWops *file)
{
	Animation *a = new Animation;
	a->is_precalculated = false;

	a->name = read_string(file);

	Uint32 num_bones = SDL_ReadLE32(file);

	for (Uint32 i = 0; i < num_bones; i++) {
		Bone *b = new Bone;
		b->name = read_string(file);

		Uint32 num_frames = SDL_ReadLE32(file);

		for (Uint32 i = 0; i < num_frames; i++) {
			b->frames.push_back(read_matrix(file));
		}

		a->bones[b->name] = b;
	}

	return a;
}

void Model::read_binary_model(SDL_RWops *file)
{
	Uint32 num_animations = SDL_ReadLE32(file);
	for (size_t i = 0; i < num_animations; i++) {
		Animation *a = read_binary_animation(file);
		instance->animations[a->name] = a;
	}

	Uint32 num_roots = SDL_ReadLE32(file);
	for (size_t i = 0; i < num_roots; i++) {
		Node *n = read_binary_frame(file);
		n->parent = 0;
		roots.push_back(n);
	}

	instance->frames_per_second = SDL_ReadLE32(file);
}

void Model::write_string(SDL_RWops *file, std::string s)
{
	Uint32 len = (Uint32)s.length();
	SDL_WriteLE32(file, len);
	for (size_t i = 0; i < len; i++) {
		util::SDL_fputc(s[i], file);
	}
}

void Model::write_matrix(SDL_RWops *file, glm::mat4 &matrix)
{
	union {
		float f;
		Uint32 u;
	} u;

	float *f = (float *)glm::value_ptr(matrix);

	for (int i = 0; i < 16; i++) {
		u.f = f[i];
		SDL_WriteLE32(file, u.u);
	}
}

void Model::save_binary_frame(SDL_RWops *file, Node *n)
{
	union {
		float f;
		Uint32 u;
	} u;

	write_string(file, n->name);

	Uint32 num_children = (Uint32)n->children.size();
	SDL_WriteLE32(file, num_children);

	for (size_t i = 0; i < num_children; i++) {
		save_binary_frame(file, n->children[i]);
	}

	write_matrix(file, n->transform);

	Uint32 num_vertices = n->num_vertices;
	SDL_WriteLE32(file, num_vertices);
	
	Uint32 num_floats = num_vertices * 12;

	for (Uint32 i = 0; i < num_floats; i++) {
		u.f = n->vertices[i];
		SDL_WriteLE32(file, u.u);
	}

	Uint32 num_textures = (Uint32)n->textures.size();
	SDL_WriteLE32(file, num_textures);

	for (Uint32 i = 0; i < num_textures; i++) {
		write_string(file, n->textures[i]->filename);
	}

	Uint32 num_triangles = n->num_triangles;
	SDL_WriteLE32(file, num_triangles);

	if (num_textures > 0) {
		for (Uint32 i = 0; i < num_triangles; i++) {
			Uint32 u = n->face_textures[i];
			SDL_WriteLE32(file, u);
		}
	}

	Uint32 num_weights = (Uint32)n->weights.size();
	SDL_WriteLE32(file, num_weights);

	for (Uint32 i = 0; i < num_weights; i++) {
		write_string(file, n->weights[i]->name);
		for (Uint32 j = 0; j < num_vertices; j++) {
			u.f = n->weights[i]->weights[j];
			SDL_WriteLE32(file, u.u);
		}
		write_matrix(file, n->weights[i]->transform);
	}

	std::vector<Bone *> bones;
	for (std::map<std::string, Animation *>::iterator it = instance->animations.begin(); it != instance->animations.end(); it++) {
		std::pair<std::string, Animation *> p = *it;
		Animation *a = p.second;
		for (std::map<std::string, Bone *>::iterator it2 = a->bones.begin(); it2 != a->bones.end(); it2++) {
			std::pair<std::string, Bone *> p2 = *it2;
			bones.push_back(p2.second);
		}
	}

	for (Uint32 i = 0; i < num_vertices; i++) {
		Uint32 num_weights = (Uint32)n->influences[i].weights.size();
		SDL_WriteLE32(file, num_weights);
		for (size_t j = 0; j < num_weights; j++) {
			Weights *w = n->influences[i].weights[j];
			Uint32 index = 0;
			for (size_t k = 0; k < n->weights.size(); k++) {
				if (w == n->weights[k]) {
					index = (Uint32)k;
					break;
				}
			}
			SDL_WriteLE32(file, index);
		}
		Uint32 num_bones = (Uint32)n->influences[i].bones.size();
		SDL_WriteLE32(file, num_bones);
		for (size_t j = 0; j < num_bones; j++) {
			Bone *b = n->influences[i].bones[j];
			Uint32 index = 0;
			for (size_t k = 0; k < bones.size(); k++) {
				if (b == bones[k]) {
					index = (Uint32)k;
					break;
				}
			}
			SDL_WriteLE32(file, index);
		}
	}
}

void Model::save_binary_animation(SDL_RWops *file, Animation *a)
{
	write_string(file, a->name);

	Uint32 num_bones = (Uint32)a->bones.size();
	SDL_WriteLE32(file, num_bones);

	for (std::map<std::string, Bone *>::iterator it = a->bones.begin(); it != a->bones.end(); it++) {
		std::pair<std::string, Bone *> p = *it;
		Bone *b = p.second;
		write_string(file, b->name);

		Uint32 num_frames = (Uint32)b->frames.size();
		SDL_WriteLE32(file, num_frames);

		for (Uint32 i = 0; i < num_frames; i++) {
			write_matrix(file, b->frames[i]);
		}
	}
}

bool Model::save_binary_model(std::string filename)
{
	SDL_RWops *file = SDL_RWFromFile(filename.c_str(), "wb");

	if (file == 0) {
		return false;
	}

	// magic
	util::SDL_fputc('n', file);
	util::SDL_fputc('s', file);
	util::SDL_fputc('m', file);
	util::SDL_fputc(' ', file);

	Uint32 num_animations = (Uint32)instance->animations.size();
	SDL_WriteLE32(file, num_animations);
	for (std::map<std::string, Animation *>::iterator it = instance->animations.begin(); it != instance->animations.end(); it++) {
		std::pair<std::string, Animation *> p = *it;
		Animation *a = p.second;
		save_binary_animation(file, a);
	}

	Uint32 num_roots = (Uint32)roots.size();
	SDL_WriteLE32(file, num_roots);
	for (size_t i = 0; i < roots.size(); i++) {
		save_binary_frame(file, roots[i]);
	}

	Uint32 fps = instance->frames_per_second;
	SDL_WriteLE32(file, fps);

	SDL_RWclose(file);

	return true;
}

void Model::add_node(Node *node)
{
	roots.push_back(node);
}

void Model::set_animation(std::string name, util::Callback finished_callback, void *finished_callback_data)
{
	bool already_set = instance->current_animation == name;
	if (already_set == false) {
		instance->current_animation = name;
	}
	instance->finished_callback = finished_callback;
	instance->finished_callback_data = finished_callback_data;
	// set up influences at this time so it can be done for only 1 anim (fastest way)
	if (already_set == false && instance->current_animation != "") {
		Node *n = find("Model");
		Animation *a = get_animation(instance->current_animation);
		if (a) {
			for (int i = 0; i < n->num_vertices; i++) {
				n->influences[i].bones.clear();
				for (size_t j = 0; j < n->influences[i].weights.size(); j++) {
					n->influences[i].bones.push_back(a->bones[n->influences[i].weights[j]->name]);
				}
			}
		}
	}
}

Model::Animation *Model::get_animation(std::string name)
{
	std::map<std::string, Animation *>::iterator it = instance->animations.find(name);
	if (it != instance->animations.end()) {
		const std::pair<std::string, Animation *> &p = *it;
		return p.second;
	}
	return 0;
}

std::string Model::get_current_animation()
{
	return instance->current_animation;
}

int Model::get_current_frame()
{
	if (instance->current_animation == "") {
		return 0;
	}

	Animation *a = get_animation(instance->current_animation);

	if (a) {
		std::map<std::string, Bone *>::iterator it = a->bones.begin();
		const std::pair<std::string, Bone *> &p = *it;
		Bone *b = p.second;
		int num_frames = (int)b->frames.size();
		int frames_elapsed = instance->elapsed / (1000 / instance->frames_per_second);
		return frames_elapsed % num_frames;
	}
	else {
		return -1;
	}
}

void Model::reset()
{
	instance->elapsed = 0;
}

void Model::start()
{
	instance->started = true;
}

void Model::stop()
{
	instance->started = false;
}

float *Model::calc_frame(std::string anim_name, int frame)
{
	Model::Node *node = find("Model");
	Model::Animation *a = get_animation(anim_name);
	float *vertices;

	if (a && a->is_precalculated) {
		int actual_frame = frame * ((float)a->precalc_fps/instance->frames_per_second);
		return precalculated[anim_name][actual_frame];
	}
	else if (precalculated.find(anim_name) != precalculated.end()) {
		int actual_frame = frame * ((float)a->precalc_fps/instance->frames_per_second);
		vertices = precalculated[anim_name][actual_frame];
	}
	else {
		vertices = node->animated_vertices;
	}
	
	Model::Node *root = node;
	while (root && root->parent != 0) {
		root = root->parent;
	}
	glm::vec4 vert;
	vert.w = 1.0f;
	
	if (anim_name != "") {
		// animate it!
		if (a == 0) {
			util::errormsg("Animation %s not found in model!\n", anim_name.c_str());
			return 0;
		}
		Model::Node *armature = find("Armature");
		glm::mat4 m;
		armature->animate(a, frame, &m);
		for (int v = 0; v < node->num_vertices; v++) {
			Model::Influence *influence = &node->influences[v];
			vert.x = node->vertices[12 * v + 0];
			vert.y = node->vertices[12 * v + 1];
			vert.z = node->vertices[12 * v + 2];
			glm::vec4 transformed(0.0f, 0.0f, 0.0f, 0.0f);
			for (size_t i = 0; i < influence->weights.size(); i++) {
				Model::Weights *w = influence->weights[i];
				Model::Bone *b = influence->bones[i];
				transformed += b->combined_transform * w->transform * vert * w->weights[v];
			}
			transformed = transformed / transformed.w;
			transformed = root->transform * /*node->transform **/ transformed;
			vertices[12 * v + 0] = transformed.x;
			vertices[12 * v + 1] = transformed.y;
			vertices[12 * v + 2] = transformed.z;
		}
	}
	else {
		for (int v = 0; v < node->num_vertices; v++) {
			vert.x = node->vertices[12 * v + 0];
			vert.y = node->vertices[12 * v + 1];
			vert.z = node->vertices[12 * v + 2];
			vert = root->transform /** node->transform*/ * vert;
			vertices[12 * v + 0] = vert.x;
			vertices[12 * v + 1] = vert.y;
			vertices[12 * v + 2] = vert.z;
		}
	}

	return vertices;
}

void Model::precalculate_animation(std::string name, int fps)
{
	Model::Node *node = find("Model");
	Animation *anim = instance->animations[name];
	anim->precalc_fps = fps;
	std::pair<std::string, Bone *> p = *(anim->bones.begin());
	Bone *bone = p.second;
	size_t num_frames = bone->frames.size();
	size_t precalc_frames = num_frames * ((float)fps/instance->frames_per_second);
	float **f = new float *[precalc_frames];
	precalculated[name] = f;
	for (size_t i = 0; i < precalc_frames; i++) {
		f[i] = new float[node->num_vertices*12];
		memcpy(f[i], node->vertices, sizeof(float)*node->num_vertices*12);
		calc_frame(name, i*((float)instance->frames_per_second/fps));
	}
	anim->is_precalculated = true;
}

void Model::precalculate_animations(int fps)
{
	std::map<std::string, Animation *>::iterator it;
	for (it = instance->animations.begin(); it != instance->animations.end(); it++) {
		std::pair<std::string, Animation *> p = *it;
		set_animation(p.first);
		precalculate_animation(p.first, fps);
	}
}

void Model::draw(SDL_Colour tint, bool textured)
{
	Model::Node *node = find("Model");

	if (node) {
		int frame = get_current_frame();
		std::string anim_name = get_current_animation();
		float *vertices = calc_frame(anim_name, frame);
		if (textured && node->textures.size() > 0) {
			Shader *old_shader = shim::current_shader;

			shim::current_shader = shim::model_shader;
			shim::current_shader->use();
			shim::current_shader->set_colour("tint", tint);
			update_projection();

			for (size_t i = 0; i < node->textures.size(); i++) {
				int start = -1;
				int end = -1;

				for (size_t j = 0; j < (size_t)node->num_triangles; j++) {
					if (node->face_textures[j] == (int)i) {
						if (start == -1) {
							start = (int)j;
						}
						end = (int)j + 1;
					}
				}

				if (start >= 0 && end >= 0) {
					Vertex_Cache::instance()->start(node->textures[i]);
					Vertex_Cache::instance()->cache_3d_immediate(vertices+start*12*3, end-start);
				}
			}

			shim::current_shader = old_shader;
			shim::current_shader->use();
			gfx::update_projection();
		}
		else {
			Vertex_Cache::instance()->start();
			Vertex_Cache::instance()->cache_3d_immediate(vertices, node->num_triangles);
		}
	}
}

void Model::draw()
{
	draw(shim::white, false);
}

void Model::draw_tinted(SDL_Colour tint)
{
	draw(tint, false);
}

void Model::draw_textured()
{
	draw(shim::white, true);
}

void Model::draw_tinted_textured(SDL_Colour tint)
{
	draw(tint, true);
}

} // End namespace gfx

} // End namespace noo
