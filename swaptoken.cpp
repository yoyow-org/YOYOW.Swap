#include "swaptoken.hpp"
#include <cmath>

#define _SYSTEM_MANAGER_UID  12345678 // uid of system manager

#define DEFI_TYPE_LIQUIDITY 0
#define DEFI_TYPE_SWAP 1

const double _SWAP_FEE_RATE = 0.001;


//@abi action
void swaptoken::createtk( const uint64_t& issuer,const uint64_t& asset_id,const int64_t& maximum_supply,const name& tkname,const uint8_t& precision)
{
	graphene_assert( get_trx_sender() == issuer,"can't create token to other user" );

    graphene_assert( asset_id > 10000, "token asset id must bigger than 10000" );
    graphene_assert( maximum_supply > 0, "max-supply must be positive");

    graphene_assert( !token_exist(asset_id), "token with asset id already exists" );

    _statstable.emplace( issuer, [&]( auto& s ) {
        s.asset_id = asset_id;
        s.max_supply = maximum_supply;
        s.supply = 0;
        s.tkname = tkname;
        s.precision = precision;
        s.issuer  = issuer;
    });
}

//@abi action
void swaptoken::issuetk( const uint64_t& to,const uint64_t& asset_id, const int64_t& quantity, const string& memo )
{
	graphene_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	auto existing = _statstable.find( asset_id);
	graphene_assert( existing != _statstable.end(), "token with asset id does not exist, create token before issue" );
	const auto& st = *existing;
	graphene_assert( to == st.issuer, "tokens can only be issued to issuer account" );

	graphene_assert( get_trx_sender() == st.issuer ,"invalid authority" );
	graphene_assert( quantity > 0, "must issue positive quantity" );

	graphene_assert( quantity <= st.max_supply - st.supply, "quantity exceeds available supply");

	_statstable.modify( st, 0, [&]( auto& s ) {
	   s.supply += quantity;
	});

	add_balance( st.issuer,asset_id,quantity, st.issuer );
}

//@abi action
void swaptoken::retiretk( const uint64_t& asset_id, const int64_t& quantity, const string& memo )
{
	graphene_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	auto existing = _statstable.find( asset_id );
	graphene_assert( existing != _statstable.end(), "token with asset id does not exist" );
	const auto& st = *existing;

	graphene_assert( get_trx_sender() == st.issuer ,"invalid authority" );
	graphene_assert( quantity > 0, "must retire positive quantity" );


	_statstable.modify( st, 0, [&]( auto& s ) {
	   s.supply -= quantity;
	});

	sub_balance( st.issuer,asset_id,quantity );
}

//@abi action
void swaptoken::transfertk( const uint64_t&    from,
                    const uint64_t&    to,
                    const uint64_t& asset_id,
                    const int64_t& quantity,
                    const string&  memo )
{
    graphene_assert( from != to, "cannot transfer to self" );
	graphene_assert( get_trx_sender() == from,"invalid authority" );

	char toname[32];	
    graphene_assert( get_account_name_by_id(toname,32,to) != -1, "to account does not exist");
	
    graphene_assert(token_exist(asset_id),"token not exist");

    graphene_assert( quantity > 0, "must transfer positive quantity" );
    graphene_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = from;

    sub_balance( from,asset_id,quantity );
    add_balance( to,asset_id,quantity, payer );
}                    


void swaptoken::sub_balance( const uint64_t& owner, const uint64_t& asset_id, const int64_t& value ) {
   const auto& from = _acounts.get( uint64_hash(owner,asset_id) , "no balance object found" );
   graphene_assert( from.amount >= value, "overdrawn balance" );

   _acounts.modify( from, owner, [&]( auto& a ) {
		 a.amount -= value;
	  });
}

void swaptoken::add_balance( const uint64_t& owner, const uint64_t& asset_id, const int64_t& value, const uint64_t& ram_payer )
{
   auto to = _acounts.find( uint64_hash(owner,asset_id) );
   if( to == _acounts.end() ) {
	  _acounts.emplace( ram_payer, [&]( auto& a ){
        a.uid = owner;
        a.asset_id = asset_id;
        a.amount = value;
	  });
   } else {
	  _acounts.modify( to, 0, [&]( auto& a ) {
        a.amount += value;
	  });
   }
}

const uint64_t swaptoken::get_balance(const uint64_t& account, const uint64_t& asset_type)const
{
//    accounts acnts( _self, account );
    auto itr = _acounts.find( uint64_hash(account,asset_type) );
    if( itr == _acounts.end() )
        return 0;
    else
        return itr->amount;
}

const bool  swaptoken::token_exist(const uint64_t& token)const
{
    auto ptr = _statstable.find(token);
    if( ptr == _statstable.end())
        return false;
    else
        return true;
}

void swaptoken::newliquidity(const uint64_t& account, const uint64_t& tokenA, const uint64_t& tokenB)
{
    graphene_assert( get_trx_sender() == account,"can't create token to other user" );

    uint64_t token1 = tokenA,token2 = tokenB;

    if(tokenA > tokenB)
        std::swap(token1,token2);

    graphene_assert(token1 != token2, "same token");

    graphene_assert(token_exist(token1),"token1 not exist");
    graphene_assert(token_exist(token2),"token2 not exist");
    
    uint64_t liquidity_id = uint64_hash(token1,token2);
    graphene_assert(_defi_liquidity.find(liquidity_id)  == _defi_liquidity.end(),"pair exists");  
    
    _defi_liquidity.emplace(account, [&](auto &t) {
        t.liquidity_id = liquidity_id;
        t.token1 = token1;
        t.token2 = token2;
        t.liquidity_token = 0;
        t.swap_weight = 0;
        t.liquidity_weight = 0;
        t.timestamp = get_head_block_time();
    });
}


void swaptoken::_swaplog(const uint64_t& account, const uint64_t& liquidity_id, const uint64_t& in_token, const uint64_t& out_token, const uint64_t& in_asset, const uint64_t& out_asset,const uint64_t& fee,const double& price)
{
    uint64_t swap_id = _swap_log.available_primary_key();
        
    _swap_log.emplace(account, [&](auto &t) {
        t.swap_id = swap_id;
        t.account = account;
        t.liquidity_id = liquidity_id;
        t.in_token = in_token;
        t.out_token = out_token;
        t.in_asset = in_asset;
        t.out_asset = out_asset;
        t.price = price;
        t.timestamp = get_head_block_time();
    });
}

void swaptoken::_liquiditylog(const uint64_t& account, const uint64_t& liquidity_id, string type, const uint64_t& in_token, const uint64_t& out_token, const uint64_t& in_asset, const uint64_t& out_asset, const uint64_t& liquidity_token)
{
    uint64_t log_id = _liquidity_log.available_primary_key();
   
    _liquidity_log.emplace(account, [&](auto &t) {
        t.log_id = log_id;
        t.account = account;
        t.liquidity_id = liquidity_id;
        t.in_token = in_token;
        t.out_token = out_token;
        t.in_asset = in_asset;
        t.out_asset = out_asset;
        t.liquidity_token = liquidity_token;
        t.type = type;
        t.timestamp = get_head_block_time();
    });
}

void swaptoken::doswap(const uint64_t& account,const uint64_t& tokenA,const uint64_t& quantityA,const uint64_t& tokenB)
{
    graphene_assert(get_trx_sender() == account,"missing authority");

    uint64_t liquidity_id = 0;
    if(tokenA > tokenB)
        liquidity_id = uint64_hash(tokenA,tokenB);
    else
        liquidity_id = uint64_hash(tokenB,tokenA);

    auto defi_liquidity = _defi_liquidity.find(liquidity_id);
    graphene_assert(defi_liquidity != _defi_liquidity.end(), "Liquidity does not exist");
    graphene_assert(defi_liquidity->liquidity_token > 0, "Liquidity is empty");

    uint64_t quantityB = 0;

    if (tokenA == defi_liquidity->token1)
    {
        double alpha = (1.0 * quantityA) / defi_liquidity->quantity1;
        double r = 1 - _SWAP_FEE_RATE;

        quantityB = (1.0 * (alpha * r) / (1 + alpha * r)) * defi_liquidity->quantity2;

        _defi_liquidity.modify(defi_liquidity, 0, [&](auto &t) {
            t.quantity1 += quantityA;
            t.quantity2 -= quantityB;
            t.price1 = 1.0 * t.quantity2 / t.quantity1;
            t.price2 = 1.0 * t.quantity1 / t.quantity2;
        });
    }
    else
    {
        double alpha = (1.0 * quantityA) / defi_liquidity->quantity2;
        double r = 1.0 - _SWAP_FEE_RATE;
        quantityB = (((alpha * r) / (1 + alpha * r)) * defi_liquidity->quantity1);
        
        _defi_liquidity.modify(defi_liquidity, 0, [&](auto &t) {
            t.quantity1 -= quantityB;
            t.quantity2 += quantityA;
            t.price1 = 1.0 * t.quantity2 / t.quantity1;
            t.price2 = 1.0 * t.quantity1 / t.quantity2;
        });
    }

    _transfer(_self,account,quantityB,tokenB, account);

    double price = (1.0 * quantityB) / quantityA;
    uint64_t fee = quantityA * _SWAP_FEE_RATE;

    this->_swaplog(account, liquidity_id, tokenA, tokenB,quantityA, quantityB, fee, price);

    this->mineswap(account,tokenA,quantityA,liquidity_id);
}

void swaptoken::addliquidity(const uint64_t& account, const uint64_t& tokenA,const uint64_t& tokenB,const uint64_t& quantityA,const uint64_t& quantityB)
{
    graphene_assert(get_trx_sender() == account,"missing authority");

    uint64_t token1 = tokenA,token2 = tokenB,quantity1 = quantityA,quantity2 = quantityB;
    if(tokenA > tokenB)
    {
        std::swap(token1,token2);
        std::swap(quantity1,quantity2);
    }

    graphene_assert(quantity1 > 0,"quantity1 == 0");
    graphene_assert(quantity2 > 0,"quantity2 == 0");

    uint64_t liquidity_id = uint64_hash(token1,token2);

    auto defi_liquidity = _defi_liquidity.find(liquidity_id);
    graphene_assert(defi_liquidity != _defi_liquidity.end(), "Liquidity does not exist");

    graphene_assert(get_balance(account,token1) >= quantity1,"balance of token1 not enough");  
    graphene_assert(get_balance(account,token2) >= quantity2,"balance of token2 not enough");  

    uint64_t pool_id = _defi_pool.available_primary_key();

    uint64_t myliquidity_token = 0;
    uint64_t liquidity_token = defi_liquidity->liquidity_token;

    if (liquidity_token == 0)
    {
        myliquidity_token = std::pow(double(quantity1) * quantity2, 0.5);
    }
    else
    {
        double price = defi_liquidity->price1;
        double mul = double(quantity1) * price;
        if (mul > quantity2)
        {
            quantity1 = quantity2 / price;
        }
        else if (mul < quantity2)
        {
            quantity2 = quantity1 * price;
        }

        double alpha = (1.00 * quantity1) / (quantity1 + defi_liquidity->quantity1);
        myliquidity_token = (alpha / (1 - alpha)) * defi_liquidity->liquidity_token;
    }
    
    liquidity_token += myliquidity_token;

    auto pool_index = _defi_pool.get_index<N(accountkey)>();
    auto pool_itr = pool_index.find(account);
    bool haspool = false;
    while (pool_itr != pool_index.end() && (pool_itr->account == account))
    {
        if (pool_itr->liquidity_id == liquidity_id)
        {
            haspool = true;
            break;
        }
        pool_itr++;
    }

    if (haspool)
    {
        pool_index.modify(pool_itr, account, [&](auto &t) {
            t.quantity1 += quantity1;
            t.quantity2 += quantity2;
            t.liquidity_token += myliquidity_token;
        });
    }
    else
    {
        _defi_pool.emplace(account, [&](auto &t) {
            t.pool_id = pool_id;
            t.liquidity_id = liquidity_id;
            t.account = account;
            t.token1 = token1;
            t.token2 = token2;
            t.quantity1 = quantity1;
            t.quantity2 = quantity2;
            t.liquidity_token = myliquidity_token;
            t.timestamp = get_head_block_time();
        });
    }


    if (defi_liquidity->liquidity_token == 0)
    {
        _defi_liquidity.modify(defi_liquidity, 0, [&](auto &t) {
            t.quantity1 = quantity1;
            t.quantity2 = quantity2;
            t.price1 = 1.0 * quantity2 / quantity1;
            t.price2 = 1.0 * quantity1 / quantity2;
            t.liquidity_token = liquidity_token;
        });
    }
    else
    {
        _defi_liquidity.modify(defi_liquidity, _self, [&](auto &t) {
            t.quantity1 += quantity1;
            t.quantity2 += quantity2;
            t.liquidity_token = liquidity_token;
        });
    }

    this->_liquiditylog(account, liquidity_id, "deposit", token1, token2, quantity1, quantity2, liquidity_token);

    _transfer(account,_self,quantity1,token1,account);
    _transfer(account,_self,quantity2,token2,account);
}

void swaptoken::subliquidity(const uint64_t& account,const uint64_t& tokenA,const uint64_t& tokenB, uint64_t liquidity_token)
{
    graphene_assert(get_trx_sender() == account,"missing authority");

    uint64_t token1 = tokenA,token2 = tokenB;
    if(tokenA > tokenB)
    {
        std::swap(token1,token2);
    }

    uint64_t liquidity_id = uint64_hash(token1,token2);

    auto defi_liquidity = _defi_liquidity.find(liquidity_id);
    graphene_assert(defi_liquidity != _defi_liquidity.end(), "Liquidity does not exist");

    auto pool_index = _defi_pool.get_index<N(accountkey)>();
    auto pool_itr = pool_index.find(account);
    bool haspool = false;
    while (pool_itr != pool_index.end() && (pool_itr->account == account))
    {
        if (pool_itr->liquidity_id == liquidity_id)
        {
            haspool = true;
            break;
        }
        pool_itr++;
    }

    graphene_assert(haspool, "User liquidity does not exist.");
    graphene_assert(pool_itr->liquidity_token >= liquidity_token, " Insufficient liquidity");

    uint64_t amount1 = (1.00 * liquidity_token / defi_liquidity->liquidity_token) * defi_liquidity->quantity1;
    uint64_t amount2 = (1.00 * liquidity_token / defi_liquidity->liquidity_token) * defi_liquidity->quantity2;
    
    graphene_assert(amount1 > 0 && amount2 > 0, "Zero");

    if (liquidity_token == pool_itr->liquidity_token)
    {
        pool_index.erase(pool_itr);
    }
    else
    {
        pool_index.modify(pool_itr, _self, [&](auto &t) {
            t.quantity1 -= amount1;
            t.quantity2 -= amount2;
            t.liquidity_token -= liquidity_token;
        });
    }
    
    if (defi_liquidity->liquidity_token == liquidity_token)
    {
        _defi_liquidity.modify(defi_liquidity, _self, [&](auto &t) {
            t.quantity1 -= amount1;
            t.quantity2 -= amount2;
            t.price1 = 0;
            t.price2 = 0;
            t.liquidity_token = 0;
        });
    }
    else
    {
        _defi_liquidity.modify(defi_liquidity, 0, [&](auto &t) {
            t.quantity1 -= amount1;
            t.quantity2 -= amount2;
            t.price1 = 1.0 * t.quantity2 /  t.quantity1;
            t.price2 = 1.0 * t.quantity1 /  t.quantity2;
            t.liquidity_token -= liquidity_token;
        });
    }

    _transfer(_self,account, amount1,token1,account);
    _transfer(_self,account, amount2,token2,account);

    this->_liquiditylog(account, liquidity_id, "withdraw", token1,token2, amount1, amount2, liquidity_token);
}

void swaptoken::mineswap(const uint64_t& account,const uint64_t& tokenA,const uint64_t& quantityA, const uint64_t& liquidity_id)
{
    auto defi_liquidity = _defi_liquidity.find(liquidity_id);

    double weight = defi_liquidity->swap_weight;

    uint64_t mine_quantity = 0;//asset(0, EOS_TOKEN_SYMBOL);

    if (tokenA == defi_liquidity->token1 )
    {
        mine_quantity = defi_liquidity->price2 * quantityA * weight;
    }
    else
    {
        mine_quantity = defi_liquidity->price1 * quantityA * weight;
    }
//todo in future
}

void swaptoken::updateweight(const uint64_t& tokenA,const uint64_t& tokenB, uint64_t type, double weight)
{
    graphene_assert(get_trx_sender() == _SYSTEM_MANAGER_UID,"missing authority");
    graphene_assert(type == DEFI_TYPE_LIQUIDITY || type == DEFI_TYPE_SWAP, "type invalid");

    uint64_t token1 = tokenA,token2 = tokenB;
    if(tokenA > tokenB)
    {
        std::swap(token1,token2);
    }
    
    uint64_t liquidity_id = uint64_hash(token1,token2);

    auto it = _defi_liquidity.find(liquidity_id);
    graphene_assert(it != _defi_liquidity.end(), "Liquidity does not exist");
    
    _defi_liquidity.modify(it, 0, [&](auto &t) 
    {
        if (type == DEFI_TYPE_LIQUIDITY)
            t.liquidity_weight = weight;
        else
            t.swap_weight = weight;
    });
}


//maximum remove 200 records per call
void swaptoken::removelog(const uint64_t& type, const uint64_t& count)
{
    graphene_assert(count > 0,"0 count");
    graphene_assert(type == DEFI_TYPE_LIQUIDITY || type == DEFI_TYPE_SWAP, "type invalid");
    graphene_assert(get_trx_sender() == _SYSTEM_MANAGER_UID,"missing authority");

    uint64_t limit = count > 200?200:count;

    uint64_t num = 0;
    
    if (type == DEFI_TYPE_LIQUIDITY)
    {
        auto it = _liquidity_log.begin();
        while (it != _liquidity_log.end())
        {
            num++;
            if (num > limit)
            {
                break;
            }
            it = _liquidity_log.erase(it);
        }
    }
    else if (type == DEFI_TYPE_SWAP)
    {
        auto it = _swap_log.begin();
        while (it != _swap_log.end())
        {

            num++;
            if (num > limit)
            {
                break;
            }
            it = _swap_log.erase(it);
        }
    }
}

void swaptoken::_transfer(const uint64_t& from,const uint64_t& to, const uint64_t& amount, const uint64_t& coin_code, const uint64_t& rampayer)
{
    sub_balance( from, coin_code,amount);
    add_balance( to, coin_code,amount,rampayer);
}

GRAPHENE_ABI(swaptoken,(createtk)(issuetk)(retiretk)(transfertk)(newliquidity)(addliquidity)(subliquidity)(doswap)(removelog)(updateweight))
