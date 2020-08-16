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
        : contract(id),
        pairs(_self, _self)
    {
    }

    //@abi action
    void createpair(const uint64_t& token0, const uint64_t& token1)
    {
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
		

        uint64_t primary_key() const { return factory; }

      
        GRAPHENE_SERIALIZE(swap_pair, (factory)(token0)(token1)(reserve0)(reserve1)(blockTimestampLast)(price0CumulativeLast)(price1CumulativeLast)(kLast))
    };

    typedef multi_index<N(swappair), swap_pair>  swap_pair_index;

    swap_pair_index pairs;
};

GRAPHENE_ABI(swap, (createpair)(setfeeto)(setfeetosetter))
