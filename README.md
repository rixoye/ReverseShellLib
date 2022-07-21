### 项目简介
本项目提供了 httpsshell 和 tcpshell的两个实现方式


### 使用方式
#### tcpshell
Client
```
#include <iostream>
#include "../../../src/httpsShellLib.h"
Client c("192.168.0.12", 888);
```
Server
```
#include <iostream>
#include "../../../src/httpsShellLib.h"
Server s(888);
```


#### httpsShell
1.使用前需要配置openssl，文档参考:https://www.jianshu.com/p/8ac6a177aa73
2. 密钥生成方法：
```
openssl genpkey -algorithm RSA -out privkey.pem -pkeyopt rsa_keygen_bits:2048
openssl req -new -x509 -key privkey.pem -out cacert.pem -days 365
```

3. Client
```
#include <iostream>
#include "../../../src/httpsShellLib.h"
HttpsClient c("10.211.55.4", 888);
```
4. Server
```
#include <iostream>
#include "../../../src/httpsShellLib.h"
HttpsServer s(888);
```




