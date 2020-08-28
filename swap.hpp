#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/contract_asset.hpp>

#include <algorithm> 

using namespace graphene;

#define SWAP_PARAM_TABLE_ROW_ID  0

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
	void initialize(uint64_t asset);
	

	//@abi action    
    void createpair(const uint64_t& tokenA, const uint64_t& tokenB);
	
    //@abi action
    void setfeeto( const uint64_t& feeto);
    
	//@abi action
    void setfeetosetter(const uint64_t& factory,const uint64_t& feetosetter);


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



	//contract token tables
	struct account {
        contract_asset    balance;

        uint64_t primary_key()const { return balance.asset_id; }
     };

     struct  currency_stats {
        contract_asset    supply;
        contract_asset    max_supply;
        uint8_t			 precision;
		name 			  tkname;
        uint64_t     issuer;

        uint64_t primary_key()const { return supply.asset_id; }

		GRAPHENE_SERIALIZE(currency_stats, (supply)(max_supply)(precision)(tkname)(issuer))
     };

     typedef multi_index< N(accounts), account > accounts;
     typedef multi_index< N(stat), currency_stats > stats;

     void sub_balance( const uint64_t& owner, const contract_asset& value );
     void add_balance( const uint64_t& owner, const contract_asset& value, const uint64_t& ram_payer );
    
};

GRAPHENE_ABI(swap, (createpair)(setfeeto)(setfeetosetter))
