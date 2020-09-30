#pragma once

#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/contract_asset.hpp>

#include <algorithm> 

using namespace graphene;
using namespace graphenelib;

using namespace std;
string to_hex(const char *d, uint32_t s)
{
    std::string r;
    const char *to_hex = "0123456789abcdef";
    uint8_t *c = (uint8_t *)d;
    for (uint32_t i = 0; i < s; ++i)
        (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
    return r;
}

uint64_t uint64_hash(const string &hash)
{
    return std::hash<string>{}(hash);
}

uint64_t uint64_hash(const uint64_t& a,const uint64_t& b)
{
	string  tmp = to_hex((const char*)&a,8);
	tmp +=to_hex((const char*)&b,8);
    return uint64_hash(tmp);
}

//contract
class swaptoken: public contract
{
public:
    swaptoken(uint64_t id)
        : contract(id),
          _defi_liquidity(_self, _self),
          _defi_pool(_self, _self),
          _liquidity_log(_self, _self),
          _swap_log(_self, _self),
          _statstable(_self, _self),
          _acounts(_self, _self)
          {};
		
	//@abi action
	void createtk( const uint64_t& issuer,const uint64_t& asset_id,const int64_t& maximum_supply,const name& tkname,const uint8_t& precision);
	//@abi action
	void issuetk( const uint64_t& to,const uint64_t& asset_id, const int64_t& quantity, const string& memo );
	//@abi action
	void retiretk( const uint64_t& asset_id, const int64_t& quantity, const string& memo );
	//@abi action
	void transfertk( const uint64_t&    from,
                    const uint64_t&    to,
                    const uint64_t& asset_id,
                    const int64_t& quantity,
                    const string&  memo );
                        
//@abi action
    void newliquidity(const uint64_t& account, const uint64_t& tokenA, const uint64_t& tokenB);
//@abi action
    void addliquidity(const uint64_t& account, const uint64_t& tokenA,const uint64_t& tokenB,const uint64_t& quantityA,const uint64_t& quantityB);
//@abi action
    void subliquidity(const uint64_t& account,const uint64_t& tokenA,const uint64_t& tokenB, uint64_t liquidity_token);
//@abi action
   void doswap(const uint64_t& account,const uint64_t& tokenA,const uint64_t& quantityA,const uint64_t& tokenB);
//@abi action
    void removelog(const uint64_t& type, const uint64_t& count);
//@abi action
    void updateweight(const uint64_t& tokenA,const uint64_t& tokenB, uint64_t type, double weight);

    void mineswap(const uint64_t& account,const uint64_t& tokenA,const uint64_t& quantityA, const uint64_t& liquidity_id);
    void _swaplog(const uint64_t& account, const uint64_t& liquidity_id, const uint64_t& in_token, const uint64_t& out_token, const uint64_t& in_asset, const uint64_t& out_asset,const uint64_t& fee,const double& price);
    void _liquiditylog(const uint64_t& account, const uint64_t& liquidity_id, string type, const uint64_t& in_token, const uint64_t& out_token, const uint64_t& in_asset, const uint64_t& out_asset, const uint64_t& liquidity_token);
private:
    void _transfer(const uint64_t& from,const uint64_t& to, const uint64_t& amount, const uint64_t& coin_code, const uint64_t& rampayer);
    const bool      token_exist(const uint64_t& token)const;
    const uint64_t get_balance(const uint64_t& account, const uint64_t& asset_type)const;
    void sub_balance( const uint64_t& owner, const uint64_t& asset_id, const int64_t& value );
    void add_balance( const uint64_t& owner, const uint64_t& asset_id, const int64_t& value, const uint64_t& ram_payer );

private:
     //@abi table dfliquidity
    struct  dfliquidity
    {
        uint64_t liquidity_id;
        uint64_t token1;
        uint64_t token2;

        uint64_t quantity1 = 0;
        uint64_t quantity2 = 0;
        uint64_t liquidity_token;
        double price1 = 0.0;
        double price2 = 0.0;
        uint64_t cumulative1 = 0;
        uint64_t cumulative2 = 0;
        double swap_weight = 0.0;
        double liquidity_weight = 0.0;
        uint64_t timestamp;

        uint64_t primary_key() const { return liquidity_id; }

        GRAPHENE_SERIALIZE(dfliquidity, (liquidity_id)(token1)(token2)(quantity1)(quantity2)(liquidity_token)(price1)(price2)(cumulative1)(cumulative2)(swap_weight)(liquidity_weight)(timestamp))
    };

    typedef multi_index<N(dfliquidity), dfliquidity> tb_defi_liquidity;

    //@abi table defipool
    struct defipool
    {
        uint64_t pool_id;
        uint64_t liquidity_id;
        uint64_t account;
        uint64_t token1;
        uint64_t token2;
        uint64_t liquidity_token;
        uint64_t quantity1;
        uint64_t quantity2;
        uint64_t timestamp;

        uint64_t primary_key() const { return pool_id; }
        uint64_t account_key() const { return account; }

        GRAPHENE_SERIALIZE(defipool, (pool_id)(liquidity_id)(account)(token1)(token2)(liquidity_token)(quantity1)(quantity2)(timestamp))
    };

    typedef multi_index<N(defipool), defipool,
                        indexed_by<N(accountkey), const_mem_fun<defipool, uint64_t, &defipool::account_key>>>
        tb_defi_pool;

    //@abi table liquiditylog
    struct  liquiditylog
    {
        uint64_t log_id;
        uint64_t account;
        uint64_t liquidity_id;
        uint64_t liquidity_token;
        uint64_t in_token;
        uint64_t out_token;
        uint64_t in_asset;
        uint64_t out_asset;
        string type;
        uint64_t timestamp;

        uint64_t primary_key() const { return log_id; }

        GRAPHENE_SERIALIZE(liquiditylog, (log_id)(account)(liquidity_id)(liquidity_token)(in_token)(out_token)(in_asset)(out_asset)(type)(timestamp))
    };

    typedef multi_index<N(liquiditylog), liquiditylog> tb_liquidity_log;

    //@abi table swaplog
    struct  swaplog
    {
        uint64_t swap_id;
        uint64_t third_id;
        uint64_t account;
        uint64_t liquidity_id;
        uint64_t in_token;
        uint64_t out_token;
        uint64_t in_asset;
        uint64_t out_asset;
        double   price;
        uint64_t timestamp;

        uint64_t primary_key() const { return swap_id; }

        GRAPHENE_SERIALIZE(swaplog, (swap_id)(third_id)(account)(liquidity_id)(in_token)(out_token)(in_asset)(out_asset)(price)(timestamp))
    };

    typedef multi_index<N(swaplog), swaplog> tb_swap_log;

    //@abi table account
	struct account {
	    uint64_t    uid;
        uint64_t    asset_id; 
        int64_t     amount;
        uint64_t primary_key()const { return uint64_hash(uid,asset_id); }

        GRAPHENE_SERIALIZE(account, (uid)(asset_id)(amount))
     };
    typedef multi_index<N(account), account> accounts;
    

    //@abi table currencysta
     struct  currencysta {
        uint64_t    asset_id;
        int64_t     supply;
        int64_t     max_supply;
        uint8_t		precision;
	    name 	    tkname;
        uint64_t    issuer;

        uint64_t primary_key()const { return asset_id; }

		GRAPHENE_SERIALIZE(currencysta, (asset_id)(supply)(max_supply)(precision)(tkname)(issuer))
     }; 
    
    typedef multi_index< N(currencysta), currencysta > stats;

    tb_defi_liquidity _defi_liquidity;
    tb_defi_pool _defi_pool;

    tb_swap_log _swap_log;
    tb_liquidity_log _liquidity_log;

    stats _statstable;
    accounts _acounts;
};


