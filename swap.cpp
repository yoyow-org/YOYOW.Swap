#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>

using namespace graphene;

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
        : contract(id)
    {
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
	}
	
    //@abi action
    void setfeeto(const uint64_t& feeto)
    {
    }
    
	//@abi action
    void setfeetosetter(const uint64_t& feetosetter)
	{
	}

  private:
    //@abi table
    struct swap_pair {
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

      
        GRAPHENE_SERIALIZE(swap_pair, (factory)(token0)(token1)(reserve0)(reserve1)(blockTimestampLast)(price0CumulativeLast)(price1CumulativeLast)(kLast))
    };

    typedef multi_index<N(swappair), swap_pair>  swap_pair_index;

	

    
};

GRAPHENE_ABI(swap, (createpair)(setfeeto)(setfeetosetter))
