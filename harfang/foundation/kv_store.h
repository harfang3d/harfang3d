// Key-value store

#include "foundation/rw_interface.h"

#include <map>
#include <string>
#include <vector>

namespace hg {

class KeyValueStore {
public:
	bool Open(const Reader &ir, const Handle &h);

	bool Open(const char *path, bool silent = false);
	bool Save(const char *path);

	bool Get(const std::string &key, std::string &value) const;
	bool Get(const std::string &key, int &value) const;
	bool Get(const std::string &key, float &value) const;
	bool Get(const std::string &key, bool &value) const;

	bool Set(const std::string &key, const std::string &value);
	bool Set(const std::string &key, const char *value);
	bool Set(const std::string &key, int value);
	bool Set(const std::string &key, float value);
	bool Set(const std::string &key, bool value);

	bool Clear(const std::string &key);

	// Return all keys storing a particular value.
	std::vector<std::string> FindValue(const std::string &value) const;

	void PushPrefix(const std::string &prefix);
	void PopPrefix();

	void Close();

private:
	std::map<std::string, std::string> kvs;

	void CommitPrefix();

	std::vector<std::string> prefix_stack;
	std::string prefix;
};

} // namespace hg
