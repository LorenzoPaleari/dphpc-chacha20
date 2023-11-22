class TestClient {
    public:
        TestClient();
        bool runTest(int i, bool (*tf)());
};

bool chacha20_rfc();
bool chacha20_rfc_text();
bool chacha20_enc_dec();