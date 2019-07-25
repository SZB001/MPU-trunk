#ifndef __KS_PSM_API_H_
#define __KS_PSM_API_H_

#define KS_API

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

//是否带有调试信息
#ifndef DEBUG
#define DEBUG
#endif

//label的最长定义
#define LABEL_NAME_LEN 64

//口令类型
#define USER_PIN 1
#define USER_PIN_COUNT  6
#define SO_PIN   0


//对称算法类型
#define ALGID_DES_ECB			0x00000001
#define ALGID_DES_ECB_PAD       0x00000002
#define ALGID_DES_CBC           0x00000003
#define ALGID_DES_CBC_PAD       0x00000004

#define ALGID_DES3_ECB          0x00000005
#define ALGID_DES3_ECB_PAD      0x00000006
#define ALGID_DES3_CBC          0x00000007
#define ALGID_DES3_CBC_PAD      0x00000008

#define ALGID_RC4                 0x00000011

#define ALGID_AES_128_ECB         0x00000012
#define ALGID_AES_128_ECB_PAD     0x00000013
#define ALGID_AES_128_CBC         0x00000014
#define ALGID_AES_128_CBC_PAD     0x00000015

#define ALGID_AES_192_ECB         0x00000016
#define ALGID_AES_192_ECB_PAD     0x00000017
#define ALGID_AES_192_CBC         0x00000018
#define ALGID_AES_192_CBC_PAD     0x00000019

#define ALGID_AES_256_ECB         0x0000001A
#define ALGID_AES_256_ECB_PAD     0x0000001B
#define ALGID_AES_256_CBC         0x0000001C
#define ALGID_AES_256_CBC_PAD     0x0000001D


//非对称算法类型
#define		ALGID_RSA1024				0x00000101		//RSA 1024位算法
#define		ALGID_RSA2048				0x00000102		//RSA 2048位算法
#define		ALGID_SM2					0x00000103		//ECC ?位算法

//摘要算法类型
#define		ALGID_HASH_SHA1					0x00000201		//SHA-1算法
#define		ALGID_HASH_SHA256				0x00000202		//SHA-256算法
#define		ALGID_HASH_SHA512				0x00000203		//SHA-512算法
#define		ALGID_HASH_SM3					0x00000204		//SM3算法
#define		ALGID_HASH_MD5					0x00000205		//MD5算法
#define		ALGID_HASH_MD4					0x00000206		//MD4算法

//
// 查找证书对应私钥签名类型
//
#define SIGN_TYPE				1 //签名证书
#define ENCRYPT_TYPE			2 //加密证书
#define SIGN_AND_ENCRYPT_TYPE	3 //既可签名又可加密证书

//
// 证书项的序号
//
#define SGD_GET_CERT_VERSION				0x00000001	//证书版本
#define SGD_GET_CERT_SERIAL					0x00000002	//证书序列号
#define SGD_GET_CERT_ISSUER					0x00000005	//证书颁发者信息
#define SGD_GET_CERT_VALID_TIME				0x00000006	//证书有效期
#define SGD_GET_CERT_NOT_BEFORE_TIME		0x00000007	//证书颁发日期
#define SGD_GET_CERT_SUBJECT				0x00000008	//证书拥有者信息
#define SGD_GET_CERT_DER_PUBLIC_KEY			0x00000009	//证书公钥信息
#define SGD_GET_CERT_DER_EXTENSIONS			0x0000000A	//证书扩展项信息
#define SGD_EXT_AUTHORITYKEYIDENTIFIER_INFO	0x00000011	//颁发者密钥标示符
#define SGD_EXT_SUBJECTKEYIDENTIFIER_INFO	0x00000012	//证书持有者密钥标示符
#define SGD_EXT_KEYUSAGE_INFO				0x00000013	//密钥用途
#define SGD_EXT_PRIVATEKEYUSAGEPERIOD_INFO	0x00000014	//私钥有效期
#define SGD_EXT_CERTIFICATEPOLICIES_INFO	0x00000015	//证书策略
#define SGD_EXT_POLICYMAPPINGS_INFO			0x00000016	//策略影射
#define SGD_EXT_BASICCONSTRAintS_INFO		0x00000017	//基本限制
#define SGD_EXT_POLICYCONSTRAintS_INFO		0x00000018	//策略限制
#define SGD_EXT_EXTKEYUSAGE_INFO			0x00000019	//扩展密钥用途
#define SGD_EXT_CRLDISTRIBUTIONPOintS_INFO	0x00000020	//CRL发布点
#define SGD_EXT_NETSCAPE_CERT_TYPE_INFO		0x00000021	//netscape属性
#define SGD_EXT_SELFDEFINED_EXTENSION_INFO	0x00000022	//私有的自定义扩展项



#ifdef __cplusplus
extern "C" {
#endif
    /*
     // C prototype : unsigned long  KS_SetContainerPath( );
     // parameter(s): pszContainerPath 绝对路径，对应根容器
     //               pszConfigFilePath 绝对路径，配置文件所在目录
     // return value: 0 - 成功;
     //				 其它值 - 错误码;
     // remarks     : 设置根容器；对于硬件版本需要且必须传入为NULL;
     //                           对于软证书版本，必须传入有效绝对路径，内部转化为必须以'\\'为结尾
     //               内部根据该值来决定调用SDKey功能还是软证书功能。
     */
    KS_API unsigned long KS_SetContainerPath
    (
     const char *pszContainerPath, // 容器根目录，该路径下文件必须有创建、删除、读、写权限（包括文件夹）
     const char *pszConfigFilePath // 配置文件的所在路径，该路径下文件必须有创建、删除、读、写权限
    );
    
    /*
     // C prototype : unsigned long  KS_Initialize( );
     // parameter(s):
     // return value: 0 - 成功;
     //				 其它值 - 错误码;
     // remarks     : 初始化p11接口，获取相关句柄等;在调用该接口之前，设备已经格式化完毕.
     */
    KS_API unsigned long KS_Initialize
    (
    
    );
    
    /*
     // C prototype : unsigned long  KS_Finalize( );
     // parameter(s):
     // return value: 0 - 成功;
     //				 其它值 - 错误码;
     // remarks     : 反初始化接口，释放相关资源.
     */
    KS_API unsigned long  KS_Finalize();
    
    /*
     // C prototype : unsigned long  KS_Login( );
     // parameter(s): [in]	  pcPin 用户PIN码
     //				 [in]	   uiPinLen 用户PIN码长度
     //				 [in/out]  puiRetryTimes返回剩余可尝试次数
     //               [in] uiLoginType 登录类型
     // return value: 0 - 成功;
     //				 其它值 - 错误码, 使用KS_GetErrorDescryption得到错误描述;
     // remarks     : 用户登录，获取使用私钥的权限.
     */
    KS_API unsigned long  KS_Login
    (
     IN const char *pcPin,
     IN unsigned int uiPinLen,
     IN OUT unsigned int *puiRetryTimes,
     IN unsigned int uiLoginType
     );
    
    
    /*
     // C prototype : unsigned long  KS_Logout( );
     // parameter(s): 无
     // return value: 0 - 成功;
     //				 其它值 - 错误码, 使用KS_GetErrorDescryption得到错误描述;
     // remarks     : 登出操作，关闭使用私钥权限.
     */
    KS_API unsigned long  KS_Logout
    (
    );
    
    /*
     // C prototype : unsigned long  KS_ChangeUserPin( );
     // parameter(s): [in]	  pcOldPin 旧PIN
     //				 [in]	   uiOldPinLen 旧PIN长度
     //				 [in] 	   pcNewPin 新PIN
     //				 [in] 	   uiNewPinLen 新PIN长度
     //				 [in/out]  puiRetryTimes返回剩余可尝试次数
     // return value: 0 - 成功;
     //				 其它值 - 错误码, 使用KS_GetErrorDescryption得到错误描述;
     // remarks     : 修改用户的PIN码，前提条件为该接口已经进行了初始化操作.
     */
    KS_API unsigned long  KS_ChangeUserPin
    (
     IN const char *pcOldPin,
     IN unsigned int uiOldPinLen,
     IN const char *pcNewPin,
     IN unsigned int uiNewPinLen,
     IN OUT unsigned int *puiRetryTimes
     );
    
    /*
     // C prototype : unsigned long  KS_UnLockUserPin( );
     // parameter(s): [in]	  	pcSoPin 管理员PIN
     //		  		[in]	   uiSoPinLen 管理员PIN的长度
     //				[in] 	   pcNewUserPin 新用户PIN
     //				[in] 	   uiNewUserPinLen 新用户PIN的长度
     //				[in/out]   puiRetryTimes 返回管理员密码剩余的尝试次数
     // return value: 0 - 成功;
     //				 其它值 - 错误码,可以使用KS_GetErrorDescryption得到错误描述;
     // remarks     : 通过sopin对设备的用户pin码进行解锁，前提条件为该接口已经进行了初始化操作.
     */
    KS_API unsigned long  KS_UnLockUserPin
    (
     IN const char *pcSoPin,
     IN unsigned int uiSoPinLen,
     IN const char *pcNewUserPin,
     IN unsigned int uiNewUserPinLen,
     IN OUT unsigned int *puiRetryTimes
     );
    
    /*
     * 管理员使用接口函数，设置sopin
     */
    KS_API unsigned long  KS_UnLockSoPin
    (
     IN const char *pcSoPin,
     IN unsigned int uiSoPinLen
     );
    
    /*
     // C prototype : unsigned long  KS_GenRandom( );
     // parameter(s): [in]    uiRandomLen要获取随机数的长度。
     //				 [in/out]	pucRandomData 生成随机数的缓冲区。
     // return value: 0 - 成功;
     //				 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 获取随机数.
     */
    KS_API unsigned long  KS_GenRandom
    (
     IN unsigned int uiRandomLen,
     IN OUT unsigned char* pucRandomData
     );
    
    
    /*
     // C prototype : unsigned long  KS_Hash( );
     // parameter(s):
     //			[in]		uiHashAlgorithmID所使用的散列算法类型，目前只支持md5、sha-1。
     //		  	[in]		pucSrcData 待做散列的源数据
     //			[in]		uiSrcDataLen 待做散列的源数据长度
     //			[out/in] 	pucHashData 输出散列值的缓冲区
     //			[out/in] 	puiHashDataLen 散列值缓冲区的长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 对数据做散列.
     */
    KS_API unsigned long   KS_Hash
    (
     IN unsigned int uiHashAlgorithmID,
     IN const unsigned char *pucSrcData,
     IN unsigned int uiSrcDataLen,
     IN OUT unsigned char *pucHashData,
     IN OUT unsigned int *puiHashDataLen
     );
    
    //
    //对于des类算法，初始化向量只需要8位，故只检查前8位是否有非0值
    //对于需要有16字节作为初始化向量部分，则检查全部16字节是否为非0值
    //
    KS_API unsigned long KS_SetIV
    (
     IN unsigned char *pucIV,
     IN unsigned int uiIVLen
     );
    
    /*
     // C prototype : unsigned long  KS_SymEncrypt( );
     // parameter(s):
     //			[in] 	pucKey密钥数据
     //			[in]	uiKeySize 返回的密钥长度
     //			[in]	uiAlgType 加密算法类型，支持DES\3DES\
     //			[in]	  	pucScrData源数据缓冲区
     //			[in]	  	uiSrcDataLen 源数据长度
     //			[in out] 	pucDestData加密结果缓冲区
     //			[in out] 	puiDestDataLen加密结果长度
     
     // return value: 0 - 成功;
     //				 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 数据对称加密.
     */
    KS_API unsigned long  KS_SymEncrypt
    (
     IN unsigned char *pucKey,
     IN unsigned int uiKeySize,
     IN unsigned int uiAlgType,
     IN unsigned char *pucScrData,
     IN unsigned int uiSrcDataLen,
     IN OUT unsigned char *pucDestData,
     IN OUT unsigned int *puiDestDataLen
     );
    
    /*
     // C prototype : unsigned long  KS_SymDecrypt( );
     // parameter(s):
     //			[in] 	pucKey密钥数据
     //			[in]	uiKeySize 返回的密钥长度
     //			[in]	uiAlgType 加密算法类型，支持DES\3DES\
     //			[in]	  	pucScrDate 密文源数据缓冲区
     //			[in]	  	uiSrcDateLen 密文源数据长度
     //			[in out] 	pucDestData 解密结果缓冲区
     //			[in out] 	puiDestDataLen 解密结果长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 数据对称解密.
     */
    KS_API unsigned long  KS_SymDecrypt
    (
     IN unsigned char *pucKey,
     IN unsigned int uiKeySize,
     IN unsigned int uiAlgType,
     IN unsigned char *pucScrDate,
     IN unsigned int uiSrcDateLen,
     IN OUT unsigned char *pucDestData,
     IN OUT unsigned int *puiDestDataLen
     );
    
    /*
     // C prototype : unsigned long  KS_UnSymEncrypt( );
     // parameter(s):
     //			[in] 	pucPublicKey 公钥数据
     //			[in]	uiPublickKeyLen 公钥长度
     //			[in]	uiAlgType 算法类型，支持RSA
     //			[in]	  	pucScrData源数据缓冲区
     //			[in]	  	uiSrcDataLen 源数据长度
     //			[in out] 	pucDestData加密结果缓冲区
     //			[in out] 	puiDestDataLen加密结果长度
     
     // return value: 0 - 成功;
     //				 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 数据非对称加密.
     */
    KS_API unsigned long  KS_UnSymEncrypt
    (
     IN const unsigned char *pucPublicKey,
     IN unsigned int uiPublickKeyLen,
     IN unsigned int uiAlgType,
     IN unsigned char *pucScrData,
     IN unsigned int uiSrcDataLen,
     IN OUT unsigned char *pucDestData,
     IN OUT unsigned int *puiDestDataLen
     );
    
    /*
     // C prototype : unsigned long  KS_UnSymDecrypt( );
     // parameter(s):
     //			[in] 	pcLable 私钥对应的label
     //			[in]	uiAlgType 算法类型，支持RSA
     //			[in]	uiCertType 证书签名类型
     //			[in]	  	pucScrData源数据缓冲区
     //			[in]	  	uiSrcDataLen 源数据长度
     //			[in out] 	pucDestData 解密结果缓冲区
     //			[in out] 	puiDestDataLen 解密结果长度
     
     // return value: 0 - 成功;
     //				 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 数据非对称解密.
     */
    KS_API unsigned long  KS_UnSymDecrypt
    (
     IN const char* pcLable,
     IN unsigned int uiAlgType,
     IN unsigned int uiCertType,
     IN unsigned char* pucScrData,
     IN unsigned int uiSrcDataLen,
     IN OUT unsigned char* pucDestData,
     IN OUT unsigned int* puiDestDataLen
     );
    
    /*
     // C prototype : unsigned long  KS_GetCert( );
     // parameter(s): [in]	      pcLable证书的Lable项
     //				 [in]         uiSignType  证书类型，签名、加密、既签名又加密
     //				 [out]        pucCertData获取的证书数据，DER编码格式
     //				 [out/in]     puiCertDataLen 获取的证书数据的长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码，用KS_GetErrorDescryption得到错误描述。;
     // remarks     : 通过证书的lable标识及对应标识数据查找证书.
     */
    KS_API unsigned long    KS_GetCert
    (
     IN const char* pcLable,
     IN unsigned int uiSignType,
     IN OUT unsigned char* pucCertData,
     IN OUT unsigned int* puiCertDataLen
     );
    
    
    /*
     // C prototype : unsigned long  KS_GetCertItem( );
     // parameter(s):
     //			[in]	  	pucCertificate证书数据
     //			[in]	  	uiCertLen证书的长度
     //			[in] 		uiInfoType证书数据索引项,参见SGD_GET_XXX的定义
     //			[in] 		pucInfo 证书索引项所对应的数据
     //			[out/in] 	puiInfoLen证书索引项数据长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码，使用KS_GetErrorDescryption得到错误描述。;
     // remarks     : 通过证书项的索引获取对应的数据.
     */
    KS_API unsigned long   KS_GetCertItem
    (
     IN unsigned char *pucCertificate,
     IN unsigned int uiCertLen,
     IN unsigned int uiInfoType,
     IN OUT unsigned char* pucInfo,
     IN OUT unsigned int* puiInfoLen
     );
    
    
    /*
     // C prototype : unsigned long  KS_GetCertItemByOid( );
     // parameter(s):
     //			[in]	  	pucCertificate证书数据
     //			[in]	  	uiCertLen证书的长度
     //			[in] 		pcOid证书数据OID索引项
     //			[in] 		pucInfo 证书索引项所对应的数据
     //			[out/in] 	puiInfoLen证书索引项数据长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码，使用KS_GetErrorDescryption得到错误描述。;
     // remarks     : 通过证书项的索引获取对应的数据.
     */
    KS_API unsigned long   KS_GetCertItemByOid
    (
     IN unsigned char *pucCertificate,
     IN unsigned int uiCertLen,
     IN char *pcOid,
     IN OUT unsigned char* pucInfo,
     IN OUT unsigned int* puiInfoLen
     );
    
    
    /*
     // C prototype : unsigned long KS_ImportCertToDevice( );
     // parameter(s):
     //			[in] pcLable证书的Lable索引
     //			[in] pucCertData 证书数据,DER编码格式
     //			[in] uiCertDataLen 获取的证书数据的长度
     //			[in] pcCertPin 证书的pin码
     //			[in] uiCertPinLen 证书的pin码长度
     //			[in] bEnableOverWrite 若证书已经存在，是否进行覆盖, 0不覆盖，其他覆盖
     //         [in] uiCertType 证书类型，1 签名， 2 加密
     // return value: 0 - 成功;
     //				 其它值 - 错误码，使用KS_GetErrorDescryption得到错误描述。;
     // remarks     : 导入证书到指定的lable中.
     */
    KS_API unsigned long   KS_ImportCertToDevice
    (
     IN const char* pcLable,
     IN unsigned char* pucCertData,
     IN unsigned int uiCertDataLen,
     IN const char* pcCertPin,
     IN unsigned int uiCertPinLen,
     IN unsigned int uiEnableOverWrite,
     IN unsigned int uiCertType
     );
    
    
    /*
     // C prototype : unsigned long  KS_GenKeyPair( );
     // parameter(s):
     //			[in]	pcLabel 私钥所使用的label,对应的是跟容器下的以szLable为文件夹名称的文件夹。
     //			[in]	uiSignType 签名、加密类型。
     //         [in]	uiAlgType 算法类型，支持RSA,ECC
     //			[in/out]	pucPublicData 返回的公钥数据，DER编码格式
     //			[in/out]	puiLen 公钥数据长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 使用硬件产生公私钥对.
     */
    KS_API unsigned long KS_GenKeyPair
    (
     IN	const char* pcLabel,
     IN unsigned int uiSignType,
     IN unsigned int uiAlgType,
     IN OUT unsigned char* pucPublicData,
     IN OUT unsigned int* puiLen
     );
    
    
    /*
     // C prototype : unsigned long  KS_MakeP10( );
     // parameter(s):
     //			[in]	pcLable 私钥所使用的label,对应的是跟容器下的以szLable为文件夹名称的文件夹。
     //			[in]	uiAlgType 算法类型，支持RSA,ECC
     //			[in]	pucDn 使用者DN项
     //			[in]	uiDnLen 使用者DN项长度
     //         [in]    pucAttrs 扩展项
     //         [in]    uiAttrsLen 扩展项长度
     //         [in]	uiHashAlg P10中使用的摘要算法
     //			[in/out]	pucP10 返回的P10数据，DER编码格式
     //			[in/out]	puiP10Len 返回的P10数据长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 产生P10.
     */
    KS_API unsigned long KS_MakeP10
    (
        IN const char* pcLable,
        IN unsigned int uiAlgType,
        IN unsigned char* pucDn,
        IN unsigned int uiDnLen,
        IN unsigned char* pucAttrs,
        IN unsigned int uiAttrsLen,
        IN unsigned int uiHashAlg,
        IN OUT unsigned char* pucP10,
        IN OUT unsigned int* puiP10Len
    );
    
    
    /*
     // C prototype : unsigned long KS_PutCert( );
     // parameter(s):
     //			[in]	  pcLable证书的Lable索引
     //          [in]	  uiSignType证书的签名、加密类型
     //          [in] pucCertData 证书数据，DER编码格式
     //          [in] uiCertDataLen 证书数据的长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码，使用KS_GetErrorDescryption得到错误描述。;
     // remarks     : 通过Key内生成公私钥对后，递交到服务器后产生证书下发后写入到设备中，软证书应用没有该场景.
     */
    KS_API unsigned long     KS_PutCert
    (
     IN const char* pcLable,
     IN unsigned int uiSignType,
     IN unsigned char* pucCertData,
     IN unsigned int uiCertDataLen
     );
    
    /*
     // C prototype : unsigned long  KS_GetP12Cert( );
     // parameter(s):
     //            [in]    pcLable 私钥所使用的label。
     //            [in]    pcPin 私钥保护口令。
     //            [in]    uiSignType 证书类型。
     //            [in/out]    pucP12Cert 加密私钥数据，该参数传入NULL，返回所需的缓冲区长度
     //            [in/out]    puiP12CertLen 加密私钥数据长度
     // return value: 0 - 成功;
     //                 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 返回获取P12
     */
    KS_API unsigned long  KS_GetP12Cert
    (
     IN const char* pcLable,
     IN const char* pcPin,
     IN unsigned int uiSignType,
     IN OUT unsigned char* pucP12Cert,
     IN OUT unsigned int* puiP12CertLen
     );
    
    /*
     // C prototype : unsigned long  KS_PriKeySignature( );
     // parameter(s):
     //            [in]    pcLable 私钥所使用的label。
     //            [in]    uiAlgType 算法类型，支持RSA,SM2
     //            [in]    uiHashAlgorithmID hash类型。
     //            [in]    uiSignType 证书类型。
     //            [in]    pucSrcData 源数据
     //            [in]    uiScrDataLen 源数据长度
     //            [in/out]    pucSignedData 签名数据，该参数传入NULL，返回所需的缓冲区长度
     //            [in/out]    puiSignedDataLen 签名数据长度
     // return value: 0 - 成功;
     //                 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 对数据使用指定的私钥进行签名.
     */
    KS_API unsigned long  KS_PriKeySignature
    (
     IN const char* pcLable,
     IN unsigned int uiAlgType,
     IN unsigned int uiHashAlgorithmID,
     IN unsigned int uiSignType,
     IN const unsigned char* pucSrcData,
     IN unsigned int uiSrcDataLen,
     IN OUT unsigned char* pucSignedData,
     IN OUT unsigned int* puiSignedDataLen
     );

    /*
     // C prototype : unsigned long  KS_HashSignature( );
     // parameter(s):
     //            [in]    pcLable 私钥所使用的label。
     //            [in]    uiAlgType 算法类型，支持RSA,SM2
     //            [in]    uiHashAlgorithmID hash类型。
     //            [in]    uiSignType 证书类型。
     //            [in]    pucHashData 源数据
     //            [in]    uiHashDataLen 源数据长度
     //            [in/out]    pucSignedData 签名数据，该参数传入NULL，返回所需的缓冲区长度
     //            [in/out]    puiSignedDataLen 签名数据长度
     // return value: 0 - 成功;
     //                 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 对数据使用指定的私钥进行签名.
     */
    KS_API unsigned long  KS_HashSignature
    (
     IN const char* pcLable,
     IN unsigned int uiAlgType,
     IN unsigned int uiHashAlgorithmID,
     IN unsigned int uiSignType,
     IN const unsigned char* pucHashData,
     IN unsigned int uiHashDataLen,
     IN OUT unsigned char* pucSignedData,
     IN OUT unsigned int* puiSignedDataLen
     );

    
    /*
     // C prototype : unsigned long  KS_Signature( );
     // parameter(s):
     //			[in]	pcLable 私钥所使用的label。
     //			[in]	uiHashAlgorithmID hash类型。
     //			[in]	uiSignType 证书类型。
     //			[in]	pucSrcData 源数据
     //			[in]	uiScrDataLen 源数据长度
     //			[in/out]	pucSignedData 签名数据，该参数传入NULL，返回所需的缓冲区长度
     //			[in/out]	puiSignedDataLen 签名数据长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 对数据使用指定的私钥进行签名.
     */
    KS_API unsigned long  KS_Signature
    (
     IN const char* pcLable,
     IN unsigned int uiHashAlgorithmID,
     IN unsigned int uiSignType,
     IN const unsigned char* pucSrcData,
     IN unsigned int uiScrDataLen,
     IN OUT unsigned char* pucSignedData,
     IN OUT unsigned int* puiSignedDataLen
     );
    
    /*
     // C prototype : unsigned long  KS_VerifySignature( );
     // parameter(s):
     //			[in] pucPublicKey所使用的公钥，der编码格式。
     //          [in] uiPublickKeyLen 公钥数据长度
     //          [in] uiHashAlgorithmID所使用的签名算法类型
     //          [in] pucSrcData 待验签的源数据
     //          [in] uiScrDataLen 待验签的源数据长度
     //          [in] pucSignedData 源数据的签名值
     //          [in] uiSignedDataLen源数据的签名值长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 使用已经签名的数据进行验签.
     */
    KS_API unsigned long  KS_VerifySignature
    (
     IN const unsigned char* pucPublicKey,
     IN unsigned int uiPublickKeyLen,
     IN unsigned int uiHashAlgorithmID,
     IN const unsigned char* pucSrcData,
     IN unsigned int uiScrDataLen,
     IN const unsigned char* pucSignedData,
     IN unsigned int uiSignedDataLen
     );

    /*
     // C prototype : unsigned long  KS_HashVerifySignature( );
     // parameter(s):
     //            [in] pucPublicKey所使用的公钥，der编码格式。
     //          [in] uiPublickKeyLen 公钥数据长度
     //          [in] uiHashAlgorithmID所使用的签名算法类型
     //          [in] pucHashData 待验签的Hash源数据
     //          [in] uiHashDataLen 待验签的Hash源数据长度
     //          [in] pucSignedData 源数据的签名值
     //          [in] uiSignedDataLen源数据的签名值长度
     // return value: 0 - 成功;
     //                 其它值 - 错误码,可以使用KS_GetErrorDescrption得到错误描述;
     // remarks     : 使用已经签名的数据进行验签.
     */
    KS_API unsigned long  KS_HashVerifySignature
    (
     IN const unsigned char* pucPublicKey,
     IN unsigned int uiPublickKeyLen,
     IN unsigned int uiHashAlgorithmID,
     IN const unsigned char* pucHashData,
     IN unsigned int uiHashDataLen,
     IN const unsigned char* pucSignedData,
     IN unsigned int uiSignedDataLen
     );
    
    /*
     // C prototype : unsigned long KS_MakeEnvelop( );
     // parameter(s):
     //                [in]   pcLable 所使用的Lable
     //				  [in]   uiCertType 证书类型
     //                [in]   pcReceiveCertData 接收者证书，DER编码格式
     //                [in]   uiReceiveCertDataLen 接收者证书长度
     //                [in]	 uiSymmAlgorithm 使用的对称算法
     //                [in]	 uiDigestAlgorithm 使用的摘要算法
     //                [in]	pucSrcData 待制作数字信封的源数据
     //                [in]	uiScrDataLen 待制作数字信封的源数据长度
     //                [out/in] pucEnvelopeData 输出数字信封的缓冲区
     //                [out/in] puiEnvelopeDataLen 数字信封缓冲区的长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码，使用KS_GetErrorDescryption得到错误描述。;
     // remarks     : 使用指定的数据制作数字信封。.
     */
    KS_API unsigned long   KS_MakeEnvelop
    (
     IN const char* pcLable,
     IN unsigned int uiCertType,
     IN const char* pcReceiveCertData,
     IN unsigned int uiReceiveCertDataLen,
     IN unsigned int uiSymmAlgorithm,
     IN unsigned int uiDigestAlgorithm,
     IN const unsigned char* pucSrcData,
     IN unsigned int uiScrDataLen,
     IN OUT unsigned char* pucEnvelopeData,
     IN OUT unsigned int * puiEnvelopeDataLen
     );
    
    /*
     // C prototype : unsigned long  KS_OpenEnvelop( );
     // parameter(s):
     //                [in]     pcLable 所使用的Lable
     //                [in]     uiCertType 发送者证书，DER格式
     //                [in]	   uiDigestAlgorithm 使用的摘要算法
     //                [in]	   pucEnvelopeData数字信封数据
     //                [in]	   uiEnvelopeDataLen数字信封数据长度
     //                [out/in] pucPlainData明文数据缓冲区
     //                [out/in] puiPlainDataLen明文数据缓冲区长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码，使用KS_GetErrorDescryption得到错误描述。;
     // remarks     : 打开数字信封，获取源数据.
     */
    
    KS_API unsigned long   KS_OpenEnvelop
    (
     IN const char* pcLable,
     IN unsigned int uiCertType,
     IN unsigned int uiDigestAlgorithm,
     IN const unsigned char* pucEnvelopeData,
     IN unsigned int uiEnvelopeDataLen,
     IN OUT unsigned char* pucPlainData,
     IN OUT unsigned int* puiPlainDataLen
     );
    
    ///////////////////////////////////////////////////////////////////////////////////////
    // C prototype : unsigned long KS_DeleteCert( );
    // parameter(s):
    //			[in]	    pcLable证书的Lable索引
    //          [in]	    uiSignType证书的签名、加密类型
    //
    // return value: 0 - 成功;
    //				 其它值 - 错误码，使用KS_GetErrorDescryption得到错误描述。;
    // remarks     : 如果存在X509证书就删除，如果有对应的密钥，则连同密钥也删除.
    ///////////////////////////////////////////////////////////////////////////////////////
    KS_API unsigned long   KS_DeleteCert
    (
     IN const char* pcLable,
     IN unsigned int uiSignType
     );
    
    /*
     // C prototype : unsigned long KS_GetLastError( );
     // parameter(s):
     // return value: 返回使用KS_SetLastError设置的错误码;
     // remarks     : 获取本接口中设置的最后错误码.
     */
    KS_API unsigned long  KS_GetLastError ( ) ;
    
    
    /*
     // C prototype : unsigned long  KS_GetLastError( );
     // parameter(s):
     // return value: 返回使用KS_SetLastError设置的错误码;
     // remarks     : 设置本接口中设置的最后错误码，对外不开放，在给用户提供的接口文档中没有该接口.
     */
    unsigned long  KS_SetLastError
    (
     IN unsigned long ulErrorCode
     );
    
    /*
     // C prototype : unsigned long  KS_GetErrorDescription( );
     // parameter(s): ulErrorCode 错误码值
     //				 pcErrInfo 用户存放错误码描述信息的缓冲区
     //				 puiLen 用户给定的缓冲区长度
     // return value: 0 - 成功;
     //				 其它值 - 错误码;
     // remarks     : 通过错误码获取错误描述信息，包括3部分，详见KS_SetLastError.
     */
    KS_API unsigned long  KS_GetErrorDescription
    (
     IN unsigned long ulErrorCode,
     IN OUT char* pcErrInfo,
     IN OUT unsigned int *puiLen
     );
    
#ifdef __cplusplus
}
#endif //

#endif


