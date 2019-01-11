#pragma once

#include "PrivateTypes.hpp"
#include "TokenExtractor.hpp"
#include "ErrorsWriter.hpp"

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

            /// \brief Switch to the next state if the next token is the expected token.
            ///
            /// Check if the next token is an opening brace or a new line.
            /// Updates the state if the next token.
            /// Generates an error if the next token is not the expected token nor a newline.
            ///
            /// \param state : will be updated if the conditions are met.
            /// \param hash : next token's hash.
            /// \param expectedHash : expected token's hash.
            /// \param token : next token.
            /// \param nextState : updated value of state if the conditions are met.
            ///
            /// \return true if there are not errors, false orherwise
            bool expectToken(uint8_t *state, StrHash hash, StrHash expectedHash, const std::string &token,
                uint8_t nextState);

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

            /// \brief Parses a labels definition block.
            /// \param interfaces : sorted vector containing interfaces to look for, or nullptr for a preset definition.
            /// \param labels : hashmap to be filled. Can be modified even if the parsing fails.
            /// \return true if successfully parsed, false otherwise.
            bool parseLabelsSubBlock(const std::vector<StrHash> *interfaces, StrHashMap<LabelInfosPrivate> &labels);

            /// \brief Parse a labels block from input stream.
            ///
            /// Parse labels data from a text input stream. Existing data will be overrided and applied to all devices,
            /// boarh already parsed and parsed afterwards.
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
            bool parseSingleBindingInput(SingleBindingInfos &binding, bool &unparsedToken, StrHash &hash,
                std::string &token);

            /// \brief Parses a full binding definition.
            ///
            /// Parse a interface's input binding. Can be several device's inputs.
            /// The function ends when it encounters a space.
            ///
            /// \param fullBinding : binding to be filled. Expected to be empty.
            /// \return true if successfully parsed, false otherwise.
            bool parseFullBindingInput(FullBindingInfos &fullBinding);

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
            /// \param interfacesDb : interfaces database to be updated if the stream is successfully parsed.
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
