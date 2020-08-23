#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
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

class swap : public contract
{
  public:
    swap(uint64_t id)
        : contract(id),
        _factories(_self,_self),
        _param(_self,_self)
    {
    }

	//@abi action
	void initialize(uint64_t asset)
	{
		graphene_assert(get_trx_sender() == _self, "only owner can initialize contract");
		graphene_assert(_param.find(SWAP_PARAM_TABLE_ROW_ID) == _param.end(), "already initialized!");
		_param.emplace(0, [&](auto &o) {
			o.swap_asset = asset;
        });	
	}

	//@abi action    
    void createpair(const uint64_t& tokenA, const uint64_t& tokenB)
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
    void setfeeto( const uint64_t& feeto)
    {
		const uint64_t& factory = get_trx_sender();
		
		auto itr = _factories.find(factory);
		graphene_assert(itr != _factories.end(), "can't find factory");

		_factories.modify(itr, 0, [&](auto &o) {
            o.feeTo += feeto;
        });
    }
    
	//@abi action
    void setfeetosetter(const uint64_t& factory,const uint64_t& feetosetter)
	{
		const uint64_t& factory = get_trx_sender();
		
		auto itr = _factories.find(factory);
		graphene_assert(itr != _factories.end(), "can't find factory");


		_factories.modify(itr, 0, [&](auto &o) {
            o.feeToSetter += feetosetter;
        });
	}

  private:
    //@abi table
    struct swap_pair_t {
        uint64_t factory;
		uint64_t token0;
		uint64_t token1;
        uint64_t reserve0;
		uint64_t reserve1;
		uint32_t blockTimestampLast;

		uint64_t price0CumulativeLast;
		uint64_t price1CumulativeLast;

		uint64_t kLast;
		

        uint64_t primary_key() const { return token1; }

		uint64_t mint(uint64_t to);


		pair<uint64_t,uint64_t> burn(uint64_t to);

		void swap(uint64_t amount0Out, uint64_t amount1Out, uint64_t to, std::vector<unsigned char> data)
		{

			graphene_assert(amount0Out > 0 || amount1Out > 0, "INSUFFICIENT_OUTPUT_AMOUNT");
			graphene_assert(amount0Out < reserve0 && amount1Out < reserve1, "INSUFFICIENT_LIQUIDITY");
		}

      
        GRAPHENE_SERIALIZE(swap_pair_t, (factory)(token0)(token1)(reserve0)(reserve1)(blockTimestampLast)(price0CumulativeLast)(price1CumulativeLast)(kLast))
    };

    typedef multi_index<N(swappair), swap_pair_t>  swap_pair_index;

	const swap_pair_t& get_pair(uint64_t token0,uint64_t token1)
	{
		if(token0 > token1)
			std::swap(token0,token1);
		
		swap_pair_index pairs(_self,token0);
		auto itr = pairs.find(token1);
		graphene_assert(itr != pairs.end(),"pair not exists");
		return *itr;
	}

	
    //@abi table
    struct factory_t {
    	uint64_t factory;
        uint64_t feeTo;
		uint64_t feeToSetter;
		uint32_t paircount;

        uint64_t primary_key() const { return factory; }

      
        GRAPHENE_SERIALIZE(factory_t, (factory)(feeTo)(feeToSetter)(paircount))
    };

    typedef multi_index<N(factory), factory_t>  factory_index;

	factory_index _factories;


	//@abi table
    struct param_t {
    	uint64_t swap_asset;

        uint64_t primary_key() const { return swap_asset; }

      
        GRAPHENE_SERIALIZE(param_t, (swap_asset))
    };

    typedef multi_index<N(param), param_t>  param_index;

	param_index _param;
    
};

GRAPHENE_ABI(swap, (createpair)(setfeeto)(setfeetosetter))
