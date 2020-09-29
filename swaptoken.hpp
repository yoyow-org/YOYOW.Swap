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
          _swap_log(_self, _self){

          };

    struct  st_defi_liquidity
    {
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

        uint64_t primary_key() const { return uint64_hash(token1,token2); }
    };

    typedef multi_index<N("liquidity"), st_defi_liquidity> tb_defi_liquidity;

    struct st_defi_pool
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
    };

    typedef multi_index<N("defipool"), st_defi_pool,
                        indexed_by<N("byaccountkey"), const_mem_fun<st_defi_pool, uint64_t, &st_defi_pool::account_key>>>
        tb_defi_pool;

    struct  st_liquidity_log
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
    };

    typedef multi_index<N("recordlog"), st_liquidity_log> tb_liquidity_log;

    struct  st_swap_log
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
        uint64_t third_key() const { return third_id; }
    };

    typedef multi_index<N("swaplog"), st_swap_log,
                        indexed_by<N("bythirdkey"), const_mem_fun<st_swap_log, uint64_t, &st_swap_log::third_key>>>
        tb_swap_log;
    
    //contract token tables
	struct account {
        contract_asset    balance;

        uint64_t primary_key()const { return balance.asset_id; }
     };

     struct  currency_stats {
        contract_asset    supply;
        contract_asset    max_supply;
        uint8_t			  precision;
		name 			  tkname;
        uint64_t     issuer;

        uint64_t primary_key()const { return supply.asset_id; }

		GRAPHENE_SERIALIZE(currency_stats, (supply)(max_supply)(precision)(tkname)(issuer))
     };

     typedef multi_index< N(accounts), account > accounts;
     typedef multi_index< N(stat), currency_stats > stats;

     void sub_balance( const uint64_t& owner, const contract_asset& value );
     void add_balance( const uint64_t& owner, const contract_asset& value, const uint64_t& ram_payer );

public:
		
		//@abi action
	void createtk( const uint64_t&   issuer,const contract_asset&  maximum_supply,const name& tkname,const uint8_t& precision);

	//@abi action
	void issuetk( const uint64_t& to, const contract_asset& quantity, const string& memo );

	//@abi action
	void retiretk( const contract_asset& quantity, const string& memo );

	//@abi action
	void transfertk( const uint64_t&    from,
                        const uint64_t&    to,
                        const contract_asset&   quantity,
                        const string&  memo );

    const bool      token_exist(const uint64_t& token)const;
    const uint64_t get_balance(const uint64_t& account, const uint64_t& asset_type)const;
                        
//@abi action
    void newliquidity(const uint64_t& account, const uint64_t& tokenA, const uint64_t& tokenB);
//@abi action
    void addliquidity(const uint64_t& account, const uint64_t& tokenA,const uint64_t& tokenB,const uint64_t& quantityA,const uint64_t& quantityB);
//@abi action
    void subliquidity(const uint64_t& account,const uint64_t& tokenA,const uint64_t& tokenB, uint64_t liquidity_token);
//@abi action
    void removelog(const uint64_t& type, const uint64_t& count);
//@abi action
    void updateweight(const uint64_t& tokenA,const uint64_t& tokenB, uint64_t type, double weight);

    void mineswap(const uint64_t& account,const uint64_t& tokenA,const uint64_t& quantityA, const uint64_t& liquidity_id);

    void doswap(const uint64_t& account,const uint64_t& tokenA,const uint64_t& quantityA,const uint64_t& tokenB);

    void _swaplog(const uint64_t& account, const uint64_t& liquidity_id, const uint64_t& in_token, const uint64_t& out_token, const uint64_t& in_asset, const uint64_t& out_asset,const uint64_t& fee,const double& price);

    void _liquiditylog(const uint64_t& account, const uint64_t& liquidity_id, string type, const uint64_t& in_token, const uint64_t& out_token, const uint64_t& in_asset, const uint64_t& out_asset, const uint64_t& liquidity_token);


private:
    void _transfer(const uint64_t& from,const uint64_t& to, const uint64_t& amount, const uint64_t& coin_code, const uint64_t& rampayer);
    
public:
    static uint64_t code;

private:
    tb_defi_liquidity _defi_liquidity;
    tb_defi_pool _defi_pool;

    tb_swap_log _swap_log;
    tb_liquidity_log _liquidity_log;
};


