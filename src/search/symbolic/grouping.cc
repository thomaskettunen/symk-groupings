#include "grouping.h"
#include "search_algorithms/symbolic_search.h"

namespace grouping {
    static class GroupingFunctionCategoryPlugin: public plugins::TypedCategoryPlugin<GroupingFunction> {
    public:
        GroupingFunctionCategoryPlugin(): TypedCategoryPlugin("GroupingFunction") {
            document_synopsis("Grouping Function");
        }
    } _category_plugin;

    class IdGroupingFeature: public plugins::TypedFeature<GroupingFunction, Id> {
    public:
        IdGroupingFeature(): TypedFeature("id") {
            symbolic::SymbolicSearch::add_options_to_feature(*this);
            document_title("Id grouping function");
            document_synopsis("for use in top-k symbolic searches, performs no grouping");
        }

        virtual std::shared_ptr<Id> create_component(const plugins::Options &options) const override {
            auto grouping_func = std::make_shared<Id>(TaskProxy(*options.get<std::shared_ptr<AbstractTask>>("transform")));
            utils::g_log << "Grouping function: " << grouping_func->to_string() << std::endl;
            return grouping_func;
        }
    };
    static plugins::FeaturePlugin<IdGroupingFeature> _id_plugin;

    class PrefixGroupingFeature: public plugins::TypedFeature<GroupingFunction, Prefix> {
    public:
        PrefixGroupingFeature(): TypedFeature("prefix") {
            document_title("Prefix grouping function");
            document_synopsis("for use in top-k symbolic searches");
            this->add_option<int>("n", "prefix length", "1");
        }

        virtual std::shared_ptr<Prefix> create_component(const plugins::Options &options) const override {
            auto grouping_func = std::make_shared<Prefix>(TaskProxy(*options.get<std::shared_ptr<AbstractTask>>("transform")), options.get<int>("n"));
            utils::g_log << "Grouping function: " << grouping_func->to_string() << std::endl;
            return grouping_func;
        }
    };
    static plugins::FeaturePlugin<PrefixGroupingFeature> _prefix_plugin;
}
