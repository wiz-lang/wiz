#ifndef WIZ_FORMAT_OUTPUT_SMS_OUTPUT_FORMAT_H
#define WIZ_FORMAT_OUTPUT_SMS_OUTPUT_FORMAT_H

#include <wiz/format/output/output_format.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    class SmsOutputFormat : public OutputFormat {
        public:
            enum class SystemType {
                MasterSystem,
                GameGear
            };

            SmsOutputFormat(SystemType systemType);
            ~SmsOutputFormat() override;

            bool generate(OutputFormatContext& context) override;
        private:
            SystemType systemType;
    };
}

#endif