#pragma once

// TODO: move it to the tools
#define TEST(name) { \
cout << __FILE__ << ":" << __LINE__ << " " #name << ": " << flush; \
name(); \
cout << "[ok]" << endl << flush; \
}
