using namespace Lazynput::Litterals;

namespace Lazynput
{
    template<class ForwardIteratorType>
    Device LazynputDb::getDevice(HidIds ids, ForwardIteratorType configTagsBegin, ForwardIteratorType configTagsEnd)
    const
    {
        // TODO: handle duplicate tags
        std::vector<StrHash> configTags = globalConfigTags;
        for(auto it = configTagsBegin; it != configTagsEnd; ++it)
            configTags.push_back(StrHash::make(*it));
        return getDevice(ids, configTags);
    }
}
