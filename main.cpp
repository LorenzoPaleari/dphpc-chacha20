#include "include/chacha.hpp"
#include "include/test_client.hpp"
#include <iostream>
using namespace std;

int main(int argc, char const *argv[])
{
    cout << "argc: " << argc << endl;
    cout << "argv[0]: " << argv[0] << endl;

    TestClient test_client = TestClient();
    (void)test_client;
    return 0;
}