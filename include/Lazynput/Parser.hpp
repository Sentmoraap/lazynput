#pragma once

#include "Lazynput/PrivateTypes.hpp"
#include "Lazynput/TokenExtractor.hpp"
#include "Lazynput/ErrorsWriter.hpp"

namespace Lazynput
{
    /// \class Parser
    /// \brief Parses data an input stream.
    class Parser
    {
        private:
            /// Writes parsing errors to the errors stream.
            ErrorsWriter errorsWriter;

            /// Extracts tokens from the input stream.
            TokenExtractor extractor;

            /// Temporary container for new devices database. Is discarded on any error encountered.
            DevicesDb newDevicesDb;

            /// Old devices database to check for redefinitions and to use previously parsed labels presets.
            DevicesDb &oldDevicesDb;

            /// Which interface’s input axis halves this input defines. The default is FULL. When the interface’s input
            /// is positive-only it must be the default value.
            enum AxisHalves : uint8_t
            {
                NEGATIVE_ONLY, /// Only the negative half
                POSITIVE_ONLY, /// Only the positive half
                FULL /// Both halves
            };

            /// \brief Switch to the next state if the next token is the expected token.
            ///
            /// Check if the next token is an opening brace or a new line.
            /// Updates the state if the next token.
            /// Generates an error if the next token is not the expected token nor a newline.
            ///
            /// \param state : will be updated if the conditions are met.
            /// \param hash : next token's hash.
            /// \param expectedHash : expected token's hash.
            /// \param skipNewLines : skip "\n" tokens and check the next non-newline token
            /// \param token : next token.
            /// \param nextState : updated value of state if the conditions are met.
            ///
            /// \return true if there are not errors, false orherwise
            bool expectToken(uint8_t *state, StrHash hash, StrHash expectedHash, bool skipNewLine,
                    std::string token, uint8_t nextState);

            /// \brief Returns the newest interface if it exists or nullprt.
            ///
            /// Checks if the interface exists in either newDevicesDb and oldDevicesDb. Returns in priority the once
            /// in newDevicesDb. It it does not exist in any database, returns nullptr.
            ///
            /// \param hash : the interface's hash to look for.
            ///
            /// \return found interface or nullptr
            Interface *getInterface(StrHash hash);

            /// \brief Returns the interface containing the input, or ""_hash.
            ///
            /// Checks if the input belongs to one and only one interface in interfaces. If so, returns it.
            /// If not, it prints an error and returns {""_hash, nullptr}.
            ///
            /// \param interfaces : interfaces to check.
            /// \param inputHash : input to look for in the interfaces.
            /// \param inputStr : input name to print in the error.
            /// \return the interface hash, or {""_hash, nullptr}.
            std::pair<StrHash, Interface*> getInputInterface(const std::vector<StrHash> &interfaces,
                    StrHash inputHash, const std::string &inputStr);

            /// \brief Parse an interfaces block.
            ///
            /// Parse interfaces data from a text input stream. Conflicting interface declarations will generate errors.
            ///
            /// \return true if successfully parsed, false otherwise.
            bool parseInterfacesBlock();

            /// \brief Parse an icons block.
            ///
            /// Parse icons data from a text input stream. Existing data will be overrided.
            ///
            /// \return true if successfully parsed, false otherwise.
            bool parseIconsBlock();

            /// \brief Parses a labels definition block.
            /// \param interfaces : sorted vector containing interfaces to look for, or nullptr for a preset definition.
            /// \param labels : hashmap to be filled. Can be modified even if the parsing fails.
            /// \return true if successfully parsed, false otherwise.
            bool parseLabelsSubBlock(const std::vector<StrHash> *interfaces, StrHashMap<DbLabelInfos> &labels);

            /// \brief Parse a labels block from input stream.
            ///
            /// Parse labels data from a text input stream. Existing data will be overrided and applied to all devices,
            /// both already parsed and parsed afterwards.
            ///
            /// \return true if successfully parsed, false otherwise.
            bool parseLabelsBlock();

            /// \brief Parses a single binding definition.
            ///
            /// Parse a single device's input binding, from possibly several device's bindings for one interfaces
            /// binding. If it finds a token which does not belong to a single binding definition, it passes it to the
            /// caller function.
            ///
            /// \param fullBinding : binding to be filled.
            /// \param unparsedToken : set to true if the function got a token to be parsed by the caller.
            /// \param hash : hash of the unparsed token
            /// \param token : unparsed token.
            /// \return true if successfully parsed, false otherwise.
            bool parseSingleBindingInput(SingleBindingInfos &positive, SingleBindingInfos *negative,
                    bool &unparsedToken, StrHash &hash, std::string &token);

            /// \brief Parses a both halves of binding definition.
            ///
            /// Parse a interface's input half binding. Can be several device's inputs.
            /// Half an axis if it can have negative value, or the full input if it’s positive-only.
            /// For bindings that has negative values, defines the bindings for both halves. In that case, it binds a
            /// full axis, i.e. the negative part is the positive part mirrored.
            /// The function ends when it encounters a space.
            ///
            /// \param positive : positive binding to be filled. Expected to be empty.
            /// \param negative : negative binding to be filled if provided. Can be null. Expected to be empty.
            /// \return true if successfully parsed, false otherwise.
            bool parseHalvesBindingInput(HalfBindingInfos &positive, HalfBindingInfos *negative);

            /// \brief Decompose bindinf infos depending of the input types and parse the halves.
            ///
            /// Parse a interface's input binding. Can be several device's inputs.
            /// Depending on the input type, the binding spans only the positive half or both halves.
            /// For bindings that has negative values, defines the bindings for both halves. In that case, it binds a
            /// full axis, i.e. the negative part is the positive part mirrored.
            ///
            /// \param fullBinding : binding to be filled. Expected to be empty.
            /// \param inputType : the input type. Depending on it the full binding may or may not have a negative part.
            /// \param axisHalves : which axis halves to bind.
            /// \return true if successfully parsed, false otherwise.
            bool parseDecomposeFullBindingInput(FullBindingInfos &fullBinding, InterfaceInputType inputType,
                    AxisHalves axisHalves);

            /// \brief Parse a device definition.
            ///
            /// Parse the inside of a device de1inition.
            ///
            /// \param device : the device structure to fill.
            /// \param deviceInterfaces : devices's interfces, may be not empty if the device has a parent.
            /// \return true if successfully parsed, false otherwise.
            bool parseDevice(DeviceData &device, std::vector<StrHash> deviceInterfaces);

            /// \brief Parses a devices block from input stream.
            ///
            /// Parse devices data from a text input stream. Existing data will be overrided.
            ///
            /// \return true if successfully parsed, false otherwise.
            bool parseDevicesBlock();

        public:
            /// \brief Constructs and initializes the parser.
            /// \param inStream : a istream providing the text input to be parsed.
            /// \param errors : a stream to write parsing errors, if any. Can be null.
            /// \param devicesDb : devices database to be updated if the stream is successfully parsed.
            Parser(std::istream &inStream, std::ostream *errors, DevicesDb &devicesDb);

            /// \brief Parses the input stream
            ///
            /// Parses the input stream and updates the databases if the stream is successfully parsed.
            /// On any error encountered, every new definitions are discarded and the databases will be unchanged.
            ///
            /// \return true if successfully parsed, false otherwise.
            bool parse();

    };
}
