#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/contract_asset.hpp>

#include <algorithm> 

using namespace graphene;

#define SWAP_PARAM_TABLE_ROW_ID  0

uint64_t sqrt(uint64_t y)
{
	uint64_t z;
	if (y > 3) {
        z = y;
        uint64_t x = y / 2 + 1;
        while (x < z) {
            z = x;
            x = (y / x + x) / 2;
        }
    } else if (y != 0) {
        z = 1;
    }
	return z;
}


//@abi action
void swap::initialize(uint64_t asset)
{
	graphene_assert(get_trx_sender() == _self, "only owner can initialize contract");
	graphene_assert(_param.find(SWAP_PARAM_TABLE_ROW_ID) == _param.end(), "already initialized!");
	_param.emplace(0, [&](auto &o) {
		o.swap_asset = asset;
    });	
}

//@abi action    
void swap::createpair(const uint64_t& tokenA, const uint64_t& tokenB)
{
	graphene_assert(tokenA != tokenB, "same token");
	uint64_t token0,token1;
	if(tokenA < tokenB)
	{
		token0 = tokenA;
		token1 = tokenB; 
	}
	else
	{
		token0 = tokenB; 
		token1 = tokenA;
	}

	swap_pair_index pairs(_self,token0);
	graphene_assert(pairs.find(token1) == pairs.end(),"pair exists");

	pairs.emplace(0, [&](auto &o) {
		o.factory = get_trx_sender();
        o.token0 = token0;
        o.token1 = token1;
        o.reserve0 = 0;
		o.reserve1 = 0;
		o.blockTimestampLast = 0;
		o.price0CumulativeLast = 0;
		o.price1CumulativeLast = 0;
		o.kLast = 0;
    });	


	auto itr = _factories.find(SWAP_PARAM_TABLE_ROW_ID);
	graphene_assert(itr != _factories.end(), "contract not initialized!");


	_factories.modify(itr, 0, [&](auto &o) {
        o.paircount += 1;
    });
}

//@abi action
void swap::setfeeto( const uint64_t& feeto)
{
	const uint64_t& factory = get_trx_sender();
	
	auto itr = _factories.find(factory);
	graphene_assert(itr != _factories.end(), "can't find factory");

	_factories.modify(itr, 0, [&](auto &o) {
        o.feeTo += feeto;
    });
}

//@abi action
void swap::setfeetosetter(const uint64_t& factory,const uint64_t& feetosetter)
{
	const uint64_t& factory = get_trx_sender();
	
	auto itr = _factories.find(factory);
	graphene_assert(itr != _factories.end(), "can't find factory");


	_factories.modify(itr, 0, [&](auto &o) {
        o.feeToSetter += feetosetter;
    });
}

//@abi action
void swap::createtk( const uint64_t&   issuer,const contract_asset&  maximum_supply,const name& tkname,const uint8_t& precision)
{
	graphene_assert( get_trx_sender() == issuer,"can't create token to other user" );

    graphene_assert( maximum_supply.asset_id > 10000, "token asset id must bigger than 10000" );
    graphene_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, maximum_supply.asset_id );
    auto existing = statstable.find( maximum_supply.asset_id );
    graphene_assert( existing == statstable.end(), "token with asset id already exists" );

    statstable.emplace( issuer, [&]( auto& s ) {
       s.max_supply    = maximum_supply;
	   s.supply = contract_asset(0, maximum_supply.asset_id);
	   s.tkname = tkname;
	   s.precision = precision;
       s.issuer        = issuer;
    });
}

//@abi action
void swap::issuetk( const uint64_t& to, const contract_asset& quantity, const string& memo )
{
	graphene_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	stats statstable(_self, quantity.asset_id );
	auto existing = statstable.find( quantity.asset_id);
	graphene_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
	const auto& st = *existing;
	graphene_assert( to == st.issuer, "tokens can only be issued to issuer account" );

	graphene_assert( get_trx_sender() == st.issuer ,"invalid authority" );
	graphene_assert( quantity.amount > 0, "must issue positive quantity" );

	graphene_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

	statstable.modify( st, 0, [&]( auto& s ) {
	   s.supply += quantity;
	});

	add_balance( st.issuer, quantity, st.issuer );
}

//@abi action
void swap::retiretk( const contract_asset& quantity, const string& memo )
{
	graphene_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	stats statstable( _self, quantity.asset_id );
	auto existing = statstable.find( quantity.asset_id );
	graphene_assert( existing != statstable.end(), "token with symbol does not exist" );
	const auto& st = *existing;

	graphene_assert( get_trx_sender() == st.issuer ,"invalid authority" );
	graphene_assert( quantity.amount > 0, "must retire positive quantity" );


	statstable.modify( st, 0, [&]( auto& s ) {
	   s.supply -= quantity;
	});

	sub_balance( st.issuer, quantity );
}


//@abi action
void swap::transfertk( const uint64_t&    from,
                    const uint64_t&    to,
                    const contract_asset&   quantity,
                    const string&  memo )
{
    graphene_assert( from != to, "cannot transfer to self" );
	graphene_assert( get_trx_sender() == from,"invalid authority" );

	char[32] toname;	
    graphene_assert( get_account_name_by_id(toname,32,to) != -1, "to account does not exist");
	
    stats statstable( _self, quantity.asset_id );
    const auto& st = statstable.get( quantity.asset_id );

    graphene_assert( quantity.amount > 0, "must transfer positive quantity" );
    graphene_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = from;

    sub_balance( from, quantity );
    add_balance( to, quantity, payer );
}                    


void swap::sub_balance( const uint64_t& owner, const contract_asset& value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.asset_id, "no balance object found" );
   graphene_assert( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
		 a.balance -= value;
	  });
}

void swap::add_balance( const uint64_t& owner, const contract_asset& value, const uint64_t& ram_payer )
{
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.asset_id );
   if( to == to_acnts.end() ) {
	  to_acnts.emplace( ram_payer, [&]( auto& a ){
		a.balance = value;
	  });
   } else {
	  to_acnts.modify( to, 0, [&]( auto& a ) {
		a.balance += value;
	  });
   }
}

