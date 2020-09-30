# YOYOW.Swap
YOYOW.Swap是YOYOW基金会推出的第一个DeFi项目，YOYOW.Swap项目类似于以太坊网络上的Uniswap，是部署在YOYOW网络上的自动化闪兑服务。YOYOW.Swap将会促进YOYOW区块链上的由用户发行的智能内容代币之间的流通。YOYOW.Swap使用自动做市商(Automatic Market Maker)算法，为用户提供高效，低滑点且简易使用的去中心化自动闪兑交易服务，YOYOW.Swap允许用户自由创建交易对和为交易对提供流动性并借此赚取流动性奖励。
# 
The first DeFi project launched by the YOYOW Foundation is YOYOW.Swap, which is similar to Uniswap on the Ethereum network and is an automatic cryptocurrency swap service deployed on the YOYOW network. YOYOW.Swap will promote the circulation of smart content tokens issued by users on the YOYOW blockchain. YOYOW.Swap uses the Automatic Market Maker algorithm to provide users with efficient, low slippage and easy-to-use decentralized automatic cryptocurrency swap services. YOYOW.Swap allows users to freely create trading pairs, and provide liquidity for trading pairs and earn liquidity rewards.


命令行测试步骤说明
0,编译合约
gxx -g swaptoken.abi swaptoken.cpp
gxx -o swaptoken.wast swaptoken.cpp

1,布置合约（先将步骤0编译的swaptoken.abi和swaptoken.wasm拷贝至yoyow_client所在目录的swaptoken子目录内）
deploy_contract "testcont" 28182  316049304  0 0 ./swaptoken false true

2,创建2种token
call_contract  28182  316049304  null createtk  "{\"issuer\":\"28182\", \"asset_id\":\"10001\",\"maximum_supply\":\"1000000\",\"tkname\":\"asset1\",\"precision\":\"8\"}" false true
call_contract  28182  316049304  null createtk  "{\"issuer\":\"28182\", \"asset_id\":\"10002\",\"maximum_supply\":\"1000000\",\"tkname\":\"asset2\",\"precision\":\"8\"}" false true

3,发行2种token
call_contract  28182  316049304  null issuetk  "{\"to\":\"28182\", \"asset_id\":\"10001\",\"quantity\":\"100000\",\"memo\":\"testmemo\"}" false true
call_contract  28182  316049304  null issuetk  "{\"to\":\"28182\", \"asset_id\":\"10002\",\"quantity\":\"100000\",\"memo\":\"testmemo\"}" false true

4,转账token
call_contract  28182  316049304  null transfertk  "{\"from\":\"28182\",\"to\":\"27662\", \"asset_id\":\"10001\",\"quantity\":\"10000\",\"memo\":\"testmemo1\"}" false true
call_contract  28182  316049304  null transfertk  "{\"from\":\"28182\",\"to\":\"27447\", \"asset_id\":\"10001\",\"quantity\":\"10000\",\"memo\":\"testmemo1\"}" false true

call_contract  28182  316049304  null transfertk  "{\"from\":\"28182\",\"to\":\"27662\", \"asset_id\":\"10002\",\"quantity\":\"10000\",\"memo\":\"testmemo2\"}" false true
call_contract  28182  316049304  null transfertk  "{\"from\":\"28182\",\"to\":\"27447\", \"asset_id\":\"10002\",\"quantity\":\"10000\",\"memo\":\"testmemo2\"}" false true


5,创建交易对
call_contract  27662  316049304  null newliquidity  "{\"account\":\"27662\",\"tokenA\":\"10001\",\"tokenB\":\"10002\"}" false true


6,加入流动性
call_contract  27662  316049304  null addliquidity  "{\"account\":\"27662\",\"tokenA\":\"10001\",\"tokenB\":\"10002\",\"quantityA\":\"1000\",\"quantityB\":\"1000\"}" false true


7,退出流动性
call_contract  27662  316049304  null subliquidity  "{\"account\":\"27662\",\"tokenA\":\"10001\",\"tokenB\":\"10002\",\"liquidity_token\":\"100\"}" false true

8,do swap
call_contract  27447  316049304  null doswap  "{\"account\":\"27447\",\"tokenA\":\"10001\",\"quantityA\":\"100\",\"tokenB\":\"10002\"}" false true


*****查询相关操作*****
查询ABI:
get_account testcont

查询合约有哪些表
get_contract_tables testcont

查询已经存在的交易对
get_table_rows_ex  testcont  dfliquidity {}

查询当前存在的流动性提供者详情
get_table_rows_ex  testcont  defipool {}


查询流动性变更历史
get_table_rows_ex  testcont  liquiditylog {}

查询swap交易历史
get_table_rows_ex  testcont  swaplog {}


查询资产
get_table_rows  testcont  currencysta 0 1000000000
get_table_rows_ex  testcont  currencysta {}


查询余额
get_table_rows  testcont  account 0 1000000000
get_table_rows_ex  testcont  account {}


更新合约
update_contract  testcont null ./swaptoken false true
