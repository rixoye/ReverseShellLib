#include <iostream>
#include "../../../src/httpsShellLib.h"



// openssl genpkey -algorithm RSA -out privkey.pem -pkeyopt rsa_keygen_bits:2048
// openssl req -new -x509 -key privkey.pem -out cacert.pem -days 365

int main() {
	HttpsServer s(888);


	// Tcp shell
	//Server s(888);
	return 0;
}