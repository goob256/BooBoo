#include <shim3/shim3.h>

using namespace noo;

int main(int argc, char **argv)
{
	try {
		if (shim::static_start() == false) {
			util::errormsg("Error during initialization\n");
			return 1;
		}

		shim::argc = argc;
		shim::argv = argv;
		shim::use_cwd = true;
		shim::error_level = 3;
		shim::log_tags = false;

		if (util::start() == false) {
			util::errormsg("Error during initialization\n");
			return 1;
		}

		if (argc < 2) {
			util::infomsg("Usage: prune_sprite_json <file.json>\n");
			return 0;
		}

		util::JSON *json = new util::JSON(argv[1], true);

		util::JSON::Node *root = json->get_root();

		util::JSON::Node *frames = root->find("frames");

		if (frames != 0) {
			while (root->children.size() > 1) {
				root->children.erase(root->children.begin()+1); // delete metadata
			}

			for (size_t i = 0; i < frames->children.size(); i++) {
				frames->children[i]->key = util::itos(i);
				std::vector<util::JSON::Node *>::iterator it;
				for (it = frames->children[i]->children.begin(); it != frames->children[i]->children.end();) {
					util::JSON::Node *n = *it;
					if (n->key == "frame" || n->key == "duration") {
						it++;
					}
					else {
						it = frames->children[i]->children.erase(it);
					}
				}
			}

			std::string str = root->to_json(0);

			FILE *out = fopen(argv[1], "wb");

			fwrite(str.c_str(), str.length(), 1, out);

			fclose(out);
		}

		util::end();

		shim::static_end();
	}
	catch (util::Error e) {
		util::errormsg("Fatal error: %s\n", e.error_message.c_str());
		return 1;
	}

	return 0;
}
