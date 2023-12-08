class TestClient {
    public:
        TestClient();
        bool runTest(int i, bool (*tf)(int));
};

bool chacha20_rfc(int opt);
bool chacha20_rfc_text(int opt);
bool chacha20_enc_dec(int opt);