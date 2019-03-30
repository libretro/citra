// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <list>
#include <numeric>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <common/file_util.h>

#include "glad/glad.h"
#include "libretro.h"

#include "audio_core/libretro_sink.h"
#include "citra_libretro/citra_libretro.h"
#include "citra_libretro/core_settings.h"
#include "citra_libretro/environment.h"
#include "citra_libretro/libretro_logger.h"
#include "citra_libretro/input/input_factory.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/memory.h"
#include "core/hle/kernel/memory.h"
#include "core/loader/loader.h"
#include "core/settings.h"
#include "core/frontend/applets/default_applets.h"
#include "video_core/renderer_opengl/renderer_opengl.h"
#include "video_core/video_core.h"

class CitraLibRetro {
public:
    CitraLibRetro() : log_filter(Log::Level::Info) {}

    Log::Filter log_filter;
    std::unique_ptr<EmuWindow_LibRetro> emu_window;
    struct retro_hw_render_callback hw_render {};
};

CitraLibRetro* emu_instance;

void retro_init() {
    emu_instance = new CitraLibRetro();
    Log::Init();
    Log::SetGlobalFilter(emu_instance->log_filter);

    // Check to see if the frontend is providing us with logging functionality
    auto callback = LibRetro::GetLoggingBackend();
    if (callback != nullptr) {
        Log::AddBackend(std::make_unique<LibRetroLogger>(callback));
    } else {
        Log::AddBackend(std::make_unique<Log::ColorConsoleBackend>());
    }

    LOG_DEBUG(Frontend, "Initialising core...");

    // Set up LLE cores
    for (const auto& service_module : Service::service_module_map) {
        Settings::values.lle_modules.emplace(service_module.name, false);
    }

    // Setup default, stub handlers for HLE applets
    Frontend::RegisterDefaultApplets();

    LibRetro::Input::Init();
}

void retro_deinit() {
    LOG_DEBUG(Frontend, "Shutting down core...");
    if (Core::System::GetInstance().IsPoweredOn()) {
        Core::System::GetInstance().Shutdown();
    }

    LibRetro::Input::Shutdown();

    delete emu_instance;

    Log::Destroy();
}

unsigned retro_api_version() {
    return RETRO_API_VERSION;
}

void LibRetro::OnConfigureEnvironment() {
    static const retro_variable values[] = {
        {"citra_use_cpu_jit", "Enable CPU JIT; enabled|disabled"},
        {"citra_use_hw_renderer", "Enable hardware renderer; enabled|disabled"},
        {"citra_use_shader_jit", "Enable shader JIT; enabled|disabled"},
        {"citra_use_hw_shaders", "Enable hardware shaders; enabled|disabled"},
        {"citra_use_acc_geo_shaders", "Enable accurate geometry shaders (only for H/W shaders); enabled|disabled"},
        {"citra_use_acc_mul", "Enable accurate shaders multiplication (only for H/W shaders); enabled|disabled"},
        {"citra_resolution_factor",
         "Resolution scale factor; 1x (Native)|2x|3x|4x|5x|6x|7x|8x|9x|10x|1x custom 1920/1080 (New 3DS XL)|2x custom 1920/1080 (New 3DS XL)|4x custom 1920/1080 (New 3DS XL)"},
        {"citra_layout_option", "Screen layout positioning; Default Top-Bottom Screen|Single "
                                "Screen Only|Large Screen, Small Screen|Side by Side"},
        {"citra_swap_screen", "Prominent 3DS screen; Top|Bottom"},
        {"citra_custom_layout", "Custom layout; disabled|enabled"},
        {"custom_screen_size","Custom screen size ratio (preset or screen ratio); 16/9 preset|0 per thousand|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|64|65|66|67|68|69|70|71|72|73|74|75|76|77|78|79|80|81|82|83|84|85|86|87|88|89|90|91|92|93|94|95|96|97|98|99|100|101|102|103|104|105|106|107|108|109|110|111|112|113|114|115|116|117|118|119|120|121|122|123|124|125|126|127|128|129|130|131|132|133|134|135|136|137|138|139|140|141|142|143|144|145|146|147|148|149|150|151|152|153|154|155|156|157|158|159|160|161|162|163|164|165|166|167|168|169|170|171|172|173|174|175|176|177|178|179|180|181|182|183|184|185|186|187|188|189|190|191|192|193|194|195|196|197|198|199|200|201|202|203|204|205|206|207|208|209|210|211|212|213|214|215|216|217|218|219|220|221|222|223|224|225|226|227|228|229|230|231|232|233|234|235|236|237|238|239|240|241|242|243|244|245|246|247|248|249|250|251|252|253|254|255|256|257|258|259|260|261|262|263|264|265|266|267|268|269|270|271|272|273|274|275|276|277|278|279|280|281|282|283|284|285|286|287|288|289|290|291|292|293|294|295|296|297|298|299|300|301|302|303|304|305|306|307|308|309|310|311|312|313|314|315|316|317|318|319|320|321|322|323|324|325|326|327|328|329|330|331|332|333|334|335|336|337|338|339|340|341|342|343|344|345|346|347|348|349|350|351|352|353|354|355|356|357|358|359|360|361|362|363|364|365|366|367|368|369|370|371|372|373|374|375|376|377|378|379|380|381|382|383|384|385|386|387|388|389|390|391|392|393|394|395|396|397|398|399|400|401|402|403|404|405|406|407|408|409|410|411|412|413|414|415|416|417|418|419|420|421|422|423|424|425|426|427|428|429|430|431|432|433|434|435|436|437|438|439|440|441|442|443|444|445|446|447|448|449|450|451|452|453|454|455|456|457|458|459|460|461|462|463|464|465|466|467|468|469|470|471|472|473|474|475|476|477|478|479|480|481|482|483|484|485|486|487|488|489|490|491|492|493|494|495|496|497|498|499|500|501|502|503|504|505|506|507|508|509|510|511|512|513|514|515|516|517|518|519|520|521|522|523|524|525|526|527|528|529|530|531|532|533|534|535|536|537|538|539|540|541|542|543|544|545|546|547|548|549|550|551|552|553|554|555|556|557|558|559|560|561|562|563|564|565|566|567|568|569|570|571|572|573|574|575|576|577|578|579|580|581|582|583|584|585|586|587|588|589|590|591|592|593|594|595|596|597|598|599|600|601|602|603|604|605|606|607|608|609|610|611|612|613|614|615|616|617|618|619|620|621|622|623|624|625|626|627|628|629|630|631|632|633|634|635|636|637|638|639|640|641|642|643|644|645|646|647|648|649|650|651|652|653|654|655|656|657|658|659|660|661|662|663|664|665|666|667|668|669|670|671|672|673|674|675|676|677|678|679|680|681|682|683|684|685|686|687|688|689|690|691|692|693|694|695|696|697|698|699|700|701|702|703|704|705|706|707|708|709|710|711|712|713|714|715|716|717|718|719|720|721|722|723|724|725|726|727|728|729|730|731|732|733|734|735|736|737|738|739|740|741|742|743|744|745|746|747|748|749|750|751|752|753|754|755|756|757|758|759|760|761|762|763|764|765|766|767|768|769|770|771|772|773|774|775|776|777|778|779|780|781|782|783|784|785|786|787|788|789|790|791|792|793|794|795|796|797|798|799|800|801|802|803|804|805|806|807|808|809|810|811|812|813|814|815|816|817|818|819|820|821|822|823|824|825|826|827|828|829|830|831|832|833|834|835|836|837|838|839|840|841|842|843|844|845|846|847|848|849|850|851|852|853|854|855|856|857|858|859|860|861|862|863|864|865|866|867|868|869|870|871|872|873|874|875|876|877|878|879|880|881|882|883|884|885|886|887|888|889|890|891|892|893|894|895|896|897|898|899|900|901|902|903|904|905|906|907|908|909|910|911|912|913|914|915|916|917|918|919|920|921|922|923|924|925|926|927|928|929|930|931|932|933|934|935|936|937|938|939|940|941|942|943|944|945|946|947|948|949|950|951|952|953|954|955|956|957|958|959|960|961|962|963|964|965|966|967|968|969|970|971|972|973|974|975|976|977|978|979|980|981|982|983|984|985|986|987|988|989|990|991|992|993|994|995|996|997|998|999|1000"},
        {"custom_top_top","Custom top screen top position (preset or screen ratio); 16/9 preset|0 per thousand|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|64|65|66|67|68|69|70|71|72|73|74|75|76|77|78|79|80|81|82|83|84|85|86|87|88|89|90|91|92|93|94|95|96|97|98|99|100|101|102|103|104|105|106|107|108|109|110|111|112|113|114|115|116|117|118|119|120|121|122|123|124|125|126|127|128|129|130|131|132|133|134|135|136|137|138|139|140|141|142|143|144|145|146|147|148|149|150|151|152|153|154|155|156|157|158|159|160|161|162|163|164|165|166|167|168|169|170|171|172|173|174|175|176|177|178|179|180|181|182|183|184|185|186|187|188|189|190|191|192|193|194|195|196|197|198|199|200|201|202|203|204|205|206|207|208|209|210|211|212|213|214|215|216|217|218|219|220|221|222|223|224|225|226|227|228|229|230|231|232|233|234|235|236|237|238|239|240|241|242|243|244|245|246|247|248|249|250|251|252|253|254|255|256|257|258|259|260|261|262|263|264|265|266|267|268|269|270|271|272|273|274|275|276|277|278|279|280|281|282|283|284|285|286|287|288|289|290|291|292|293|294|295|296|297|298|299|300|301|302|303|304|305|306|307|308|309|310|311|312|313|314|315|316|317|318|319|320|321|322|323|324|325|326|327|328|329|330|331|332|333|334|335|336|337|338|339|340|341|342|343|344|345|346|347|348|349|350|351|352|353|354|355|356|357|358|359|360|361|362|363|364|365|366|367|368|369|370|371|372|373|374|375|376|377|378|379|380|381|382|383|384|385|386|387|388|389|390|391|392|393|394|395|396|397|398|399|400|401|402|403|404|405|406|407|408|409|410|411|412|413|414|415|416|417|418|419|420|421|422|423|424|425|426|427|428|429|430|431|432|433|434|435|436|437|438|439|440|441|442|443|444|445|446|447|448|449|450|451|452|453|454|455|456|457|458|459|460|461|462|463|464|465|466|467|468|469|470|471|472|473|474|475|476|477|478|479|480|481|482|483|484|485|486|487|488|489|490|491|492|493|494|495|496|497|498|499|500|501|502|503|504|505|506|507|508|509|510|511|512|513|514|515|516|517|518|519|520|521|522|523|524|525|526|527|528|529|530|531|532|533|534|535|536|537|538|539|540|541|542|543|544|545|546|547|548|549|550|551|552|553|554|555|556|557|558|559|560|561|562|563|564|565|566|567|568|569|570|571|572|573|574|575|576|577|578|579|580|581|582|583|584|585|586|587|588|589|590|591|592|593|594|595|596|597|598|599|600|601|602|603|604|605|606|607|608|609|610|611|612|613|614|615|616|617|618|619|620|621|622|623|624|625|626|627|628|629|630|631|632|633|634|635|636|637|638|639|640|641|642|643|644|645|646|647|648|649|650|651|652|653|654|655|656|657|658|659|660|661|662|663|664|665|666|667|668|669|670|671|672|673|674|675|676|677|678|679|680|681|682|683|684|685|686|687|688|689|690|691|692|693|694|695|696|697|698|699|700|701|702|703|704|705|706|707|708|709|710|711|712|713|714|715|716|717|718|719|720|721|722|723|724|725|726|727|728|729|730|731|732|733|734|735|736|737|738|739|740|741|742|743|744|745|746|747|748|749|750|751|752|753|754|755|756|757|758|759|760|761|762|763|764|765|766|767|768|769|770|771|772|773|774|775|776|777|778|779|780|781|782|783|784|785|786|787|788|789|790|791|792|793|794|795|796|797|798|799|800|801|802|803|804|805|806|807|808|809|810|811|812|813|814|815|816|817|818|819|820|821|822|823|824|825|826|827|828|829|830|831|832|833|834|835|836|837|838|839|840|841|842|843|844|845|846|847|848|849|850|851|852|853|854|855|856|857|858|859|860|861|862|863|864|865|866|867|868|869|870|871|872|873|874|875|876|877|878|879|880|881|882|883|884|885|886|887|888|889|890|891|892|893|894|895|896|897|898|899|900|901|902|903|904|905|906|907|908|909|910|911|912|913|914|915|916|917|918|919|920|921|922|923|924|925|926|927|928|929|930|931|932|933|934|935|936|937|938|939|940|941|942|943|944|945|946|947|948|949|950|951|952|953|954|955|956|957|958|959|960|961|962|963|964|965|966|967|968|969|970|971|972|973|974|975|976|977|978|979|980|981|982|983|984|985|986|987|988|989|990|991|992|993|994|995|996|997|998|999|1000"},
        {"custom_top_left","Custom top screen left position (preset or screen ratio); 16/9 preset|0 per thousand|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|64|65|66|67|68|69|70|71|72|73|74|75|76|77|78|79|80|81|82|83|84|85|86|87|88|89|90|91|92|93|94|95|96|97|98|99|100|101|102|103|104|105|106|107|108|109|110|111|112|113|114|115|116|117|118|119|120|121|122|123|124|125|126|127|128|129|130|131|132|133|134|135|136|137|138|139|140|141|142|143|144|145|146|147|148|149|150|151|152|153|154|155|156|157|158|159|160|161|162|163|164|165|166|167|168|169|170|171|172|173|174|175|176|177|178|179|180|181|182|183|184|185|186|187|188|189|190|191|192|193|194|195|196|197|198|199|200|201|202|203|204|205|206|207|208|209|210|211|212|213|214|215|216|217|218|219|220|221|222|223|224|225|226|227|228|229|230|231|232|233|234|235|236|237|238|239|240|241|242|243|244|245|246|247|248|249|250|251|252|253|254|255|256|257|258|259|260|261|262|263|264|265|266|267|268|269|270|271|272|273|274|275|276|277|278|279|280|281|282|283|284|285|286|287|288|289|290|291|292|293|294|295|296|297|298|299|300|301|302|303|304|305|306|307|308|309|310|311|312|313|314|315|316|317|318|319|320|321|322|323|324|325|326|327|328|329|330|331|332|333|334|335|336|337|338|339|340|341|342|343|344|345|346|347|348|349|350|351|352|353|354|355|356|357|358|359|360|361|362|363|364|365|366|367|368|369|370|371|372|373|374|375|376|377|378|379|380|381|382|383|384|385|386|387|388|389|390|391|392|393|394|395|396|397|398|399|400|401|402|403|404|405|406|407|408|409|410|411|412|413|414|415|416|417|418|419|420|421|422|423|424|425|426|427|428|429|430|431|432|433|434|435|436|437|438|439|440|441|442|443|444|445|446|447|448|449|450|451|452|453|454|455|456|457|458|459|460|461|462|463|464|465|466|467|468|469|470|471|472|473|474|475|476|477|478|479|480|481|482|483|484|485|486|487|488|489|490|491|492|493|494|495|496|497|498|499|500|501|502|503|504|505|506|507|508|509|510|511|512|513|514|515|516|517|518|519|520|521|522|523|524|525|526|527|528|529|530|531|532|533|534|535|536|537|538|539|540|541|542|543|544|545|546|547|548|549|550|551|552|553|554|555|556|557|558|559|560|561|562|563|564|565|566|567|568|569|570|571|572|573|574|575|576|577|578|579|580|581|582|583|584|585|586|587|588|589|590|591|592|593|594|595|596|597|598|599|600|601|602|603|604|605|606|607|608|609|610|611|612|613|614|615|616|617|618|619|620|621|622|623|624|625|626|627|628|629|630|631|632|633|634|635|636|637|638|639|640|641|642|643|644|645|646|647|648|649|650|651|652|653|654|655|656|657|658|659|660|661|662|663|664|665|666|667|668|669|670|671|672|673|674|675|676|677|678|679|680|681|682|683|684|685|686|687|688|689|690|691|692|693|694|695|696|697|698|699|700|701|702|703|704|705|706|707|708|709|710|711|712|713|714|715|716|717|718|719|720|721|722|723|724|725|726|727|728|729|730|731|732|733|734|735|736|737|738|739|740|741|742|743|744|745|746|747|748|749|750|751|752|753|754|755|756|757|758|759|760|761|762|763|764|765|766|767|768|769|770|771|772|773|774|775|776|777|778|779|780|781|782|783|784|785|786|787|788|789|790|791|792|793|794|795|796|797|798|799|800|801|802|803|804|805|806|807|808|809|810|811|812|813|814|815|816|817|818|819|820|821|822|823|824|825|826|827|828|829|830|831|832|833|834|835|836|837|838|839|840|841|842|843|844|845|846|847|848|849|850|851|852|853|854|855|856|857|858|859|860|861|862|863|864|865|866|867|868|869|870|871|872|873|874|875|876|877|878|879|880|881|882|883|884|885|886|887|888|889|890|891|892|893|894|895|896|897|898|899|900|901|902|903|904|905|906|907|908|909|910|911|912|913|914|915|916|917|918|919|920|921|922|923|924|925|926|927|928|929|930|931|932|933|934|935|936|937|938|939|940|941|942|943|944|945|946|947|948|949|950|951|952|953|954|955|956|957|958|959|960|961|962|963|964|965|966|967|968|969|970|971|972|973|974|975|976|977|978|979|980|981|982|983|984|985|986|987|988|989|990|991|992|993|994|995|996|997|998|999|1000"},
        {"custom_bottom_top","Custom bottom screen top position (preset or screen ratio); 16/9 preset|0 per thousand|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|64|65|66|67|68|69|70|71|72|73|74|75|76|77|78|79|80|81|82|83|84|85|86|87|88|89|90|91|92|93|94|95|96|97|98|99|100|101|102|103|104|105|106|107|108|109|110|111|112|113|114|115|116|117|118|119|120|121|122|123|124|125|126|127|128|129|130|131|132|133|134|135|136|137|138|139|140|141|142|143|144|145|146|147|148|149|150|151|152|153|154|155|156|157|158|159|160|161|162|163|164|165|166|167|168|169|170|171|172|173|174|175|176|177|178|179|180|181|182|183|184|185|186|187|188|189|190|191|192|193|194|195|196|197|198|199|200|201|202|203|204|205|206|207|208|209|210|211|212|213|214|215|216|217|218|219|220|221|222|223|224|225|226|227|228|229|230|231|232|233|234|235|236|237|238|239|240|241|242|243|244|245|246|247|248|249|250|251|252|253|254|255|256|257|258|259|260|261|262|263|264|265|266|267|268|269|270|271|272|273|274|275|276|277|278|279|280|281|282|283|284|285|286|287|288|289|290|291|292|293|294|295|296|297|298|299|300|301|302|303|304|305|306|307|308|309|310|311|312|313|314|315|316|317|318|319|320|321|322|323|324|325|326|327|328|329|330|331|332|333|334|335|336|337|338|339|340|341|342|343|344|345|346|347|348|349|350|351|352|353|354|355|356|357|358|359|360|361|362|363|364|365|366|367|368|369|370|371|372|373|374|375|376|377|378|379|380|381|382|383|384|385|386|387|388|389|390|391|392|393|394|395|396|397|398|399|400|401|402|403|404|405|406|407|408|409|410|411|412|413|414|415|416|417|418|419|420|421|422|423|424|425|426|427|428|429|430|431|432|433|434|435|436|437|438|439|440|441|442|443|444|445|446|447|448|449|450|451|452|453|454|455|456|457|458|459|460|461|462|463|464|465|466|467|468|469|470|471|472|473|474|475|476|477|478|479|480|481|482|483|484|485|486|487|488|489|490|491|492|493|494|495|496|497|498|499|500|501|502|503|504|505|506|507|508|509|510|511|512|513|514|515|516|517|518|519|520|521|522|523|524|525|526|527|528|529|530|531|532|533|534|535|536|537|538|539|540|541|542|543|544|545|546|547|548|549|550|551|552|553|554|555|556|557|558|559|560|561|562|563|564|565|566|567|568|569|570|571|572|573|574|575|576|577|578|579|580|581|582|583|584|585|586|587|588|589|590|591|592|593|594|595|596|597|598|599|600|601|602|603|604|605|606|607|608|609|610|611|612|613|614|615|616|617|618|619|620|621|622|623|624|625|626|627|628|629|630|631|632|633|634|635|636|637|638|639|640|641|642|643|644|645|646|647|648|649|650|651|652|653|654|655|656|657|658|659|660|661|662|663|664|665|666|667|668|669|670|671|672|673|674|675|676|677|678|679|680|681|682|683|684|685|686|687|688|689|690|691|692|693|694|695|696|697|698|699|700|701|702|703|704|705|706|707|708|709|710|711|712|713|714|715|716|717|718|719|720|721|722|723|724|725|726|727|728|729|730|731|732|733|734|735|736|737|738|739|740|741|742|743|744|745|746|747|748|749|750|751|752|753|754|755|756|757|758|759|760|761|762|763|764|765|766|767|768|769|770|771|772|773|774|775|776|777|778|779|780|781|782|783|784|785|786|787|788|789|790|791|792|793|794|795|796|797|798|799|800|801|802|803|804|805|806|807|808|809|810|811|812|813|814|815|816|817|818|819|820|821|822|823|824|825|826|827|828|829|830|831|832|833|834|835|836|837|838|839|840|841|842|843|844|845|846|847|848|849|850|851|852|853|854|855|856|857|858|859|860|861|862|863|864|865|866|867|868|869|870|871|872|873|874|875|876|877|878|879|880|881|882|883|884|885|886|887|888|889|890|891|892|893|894|895|896|897|898|899|900|901|902|903|904|905|906|907|908|909|910|911|912|913|914|915|916|917|918|919|920|921|922|923|924|925|926|927|928|929|930|931|932|933|934|935|936|937|938|939|940|941|942|943|944|945|946|947|948|949|950|951|952|953|954|955|956|957|958|959|960|961|962|963|964|965|966|967|968|969|970|971|972|973|974|975|976|977|978|979|980|981|982|983|984|985|986|987|988|989|990|991|992|993|994|995|996|997|998|999|1000"},
        {"custom_bottom_left","Custom bottom screen left position (preset or screen ratio); 16/9 preset|0 per thousand|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|64|65|66|67|68|69|70|71|72|73|74|75|76|77|78|79|80|81|82|83|84|85|86|87|88|89|90|91|92|93|94|95|96|97|98|99|100|101|102|103|104|105|106|107|108|109|110|111|112|113|114|115|116|117|118|119|120|121|122|123|124|125|126|127|128|129|130|131|132|133|134|135|136|137|138|139|140|141|142|143|144|145|146|147|148|149|150|151|152|153|154|155|156|157|158|159|160|161|162|163|164|165|166|167|168|169|170|171|172|173|174|175|176|177|178|179|180|181|182|183|184|185|186|187|188|189|190|191|192|193|194|195|196|197|198|199|200|201|202|203|204|205|206|207|208|209|210|211|212|213|214|215|216|217|218|219|220|221|222|223|224|225|226|227|228|229|230|231|232|233|234|235|236|237|238|239|240|241|242|243|244|245|246|247|248|249|250|251|252|253|254|255|256|257|258|259|260|261|262|263|264|265|266|267|268|269|270|271|272|273|274|275|276|277|278|279|280|281|282|283|284|285|286|287|288|289|290|291|292|293|294|295|296|297|298|299|300|301|302|303|304|305|306|307|308|309|310|311|312|313|314|315|316|317|318|319|320|321|322|323|324|325|326|327|328|329|330|331|332|333|334|335|336|337|338|339|340|341|342|343|344|345|346|347|348|349|350|351|352|353|354|355|356|357|358|359|360|361|362|363|364|365|366|367|368|369|370|371|372|373|374|375|376|377|378|379|380|381|382|383|384|385|386|387|388|389|390|391|392|393|394|395|396|397|398|399|400|401|402|403|404|405|406|407|408|409|410|411|412|413|414|415|416|417|418|419|420|421|422|423|424|425|426|427|428|429|430|431|432|433|434|435|436|437|438|439|440|441|442|443|444|445|446|447|448|449|450|451|452|453|454|455|456|457|458|459|460|461|462|463|464|465|466|467|468|469|470|471|472|473|474|475|476|477|478|479|480|481|482|483|484|485|486|487|488|489|490|491|492|493|494|495|496|497|498|499|500|501|502|503|504|505|506|507|508|509|510|511|512|513|514|515|516|517|518|519|520|521|522|523|524|525|526|527|528|529|530|531|532|533|534|535|536|537|538|539|540|541|542|543|544|545|546|547|548|549|550|551|552|553|554|555|556|557|558|559|560|561|562|563|564|565|566|567|568|569|570|571|572|573|574|575|576|577|578|579|580|581|582|583|584|585|586|587|588|589|590|591|592|593|594|595|596|597|598|599|600|601|602|603|604|605|606|607|608|609|610|611|612|613|614|615|616|617|618|619|620|621|622|623|624|625|626|627|628|629|630|631|632|633|634|635|636|637|638|639|640|641|642|643|644|645|646|647|648|649|650|651|652|653|654|655|656|657|658|659|660|661|662|663|664|665|666|667|668|669|670|671|672|673|674|675|676|677|678|679|680|681|682|683|684|685|686|687|688|689|690|691|692|693|694|695|696|697|698|699|700|701|702|703|704|705|706|707|708|709|710|711|712|713|714|715|716|717|718|719|720|721|722|723|724|725|726|727|728|729|730|731|732|733|734|735|736|737|738|739|740|741|742|743|744|745|746|747|748|749|750|751|752|753|754|755|756|757|758|759|760|761|762|763|764|765|766|767|768|769|770|771|772|773|774|775|776|777|778|779|780|781|782|783|784|785|786|787|788|789|790|791|792|793|794|795|796|797|798|799|800|801|802|803|804|805|806|807|808|809|810|811|812|813|814|815|816|817|818|819|820|821|822|823|824|825|826|827|828|829|830|831|832|833|834|835|836|837|838|839|840|841|842|843|844|845|846|847|848|849|850|851|852|853|854|855|856|857|858|859|860|861|862|863|864|865|866|867|868|869|870|871|872|873|874|875|876|877|878|879|880|881|882|883|884|885|886|887|888|889|890|891|892|893|894|895|896|897|898|899|900|901|902|903|904|905|906|907|908|909|910|911|912|913|914|915|916|917|918|919|920|921|922|923|924|925|926|927|928|929|930|931|932|933|934|935|936|937|938|939|940|941|942|943|944|945|946|947|948|949|950|951|952|953|954|955|956|957|958|959|960|961|962|963|964|965|966|967|968|969|970|971|972|973|974|975|976|977|978|979|980|981|982|983|984|985|986|987|988|989|990|991|992|993|994|995|996|997|998|999|1000"},
        {"citra_analog_function",
         "Right analog function; C-Stick and Touchscreen Pointer|Touchscreen Pointer|C-Stick"},
        {"citra_deadzone", "Emulated pointer deadzone (%); 15|20|25|30|35|0|5|10"},
        {"citra_use_virtual_sd", "Enable virtual SD card; enabled|disabled"},
        {"citra_use_libretro_save_path", "Savegame location; LibRetro Default|Citra Default"},
        {"citra_is_new_3ds", "3DS system model; Old 3DS|New 3DS"},
        {"citra_region_value",
         "3DS system region; Auto|Japan|USA|Europe|Australia|China|Korea|Taiwan"},
        {"citra_use_gdbstub", "Enable GDB stub; disabled|enabled"},
        {nullptr, nullptr}};

    LibRetro::SetVariables(values);

    static const struct retro_controller_description controllers[] = {
        {"Nintendo 3DS", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)},
    };

    static const struct retro_controller_info ports[] = {
        {controllers, 1},
        {nullptr, 0},
    };

    LibRetro::SetControllerInfo(ports);
}

uintptr_t LibRetro::GetFramebuffer() {
    return emu_instance->hw_render.get_current_framebuffer();
}

/**
 * Updates Citra's settings with Libretro's.
 */
void UpdateSettings() {
    struct retro_input_descriptor desc[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "ZL"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "ZR"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "Home"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "Touch Screen Touch"},
        {0, 0},
    };

    LibRetro::SetInputDescriptors(desc);

    // Some settings cannot be set by LibRetro frontends - options have to be
    // finite. Make assumptions.
    Settings::values.log_filter = "*:Info";
    Settings::values.sink_id = "libretro";
    Settings::values.volume = 1.0f;

    // We don't need these, as this is the frontend's responsibility.
    Settings::values.enable_audio_stretching = false;
    Settings::values.use_frame_limit = false;
    Settings::values.frame_limit = 100;

    // For our other settings, import them from LibRetro.
    Settings::values.use_cpu_jit =
        LibRetro::FetchVariable("citra_use_cpu_jit", "enabled") == "enabled";
    Settings::values.use_hw_renderer =
        LibRetro::FetchVariable("citra_use_hw_renderer", "enabled") == "enabled";
    Settings::values.use_hw_shader =
            LibRetro::FetchVariable("citra_use_hw_shaders", "enabled") == "enabled";
    Settings::values.use_shader_jit =
        LibRetro::FetchVariable("citra_use_shader_jit", "enabled") == "enabled";
    Settings::values.shaders_accurate_gs =
            LibRetro::FetchVariable("citra_use_acc_geo_shaders", "enabled") == "enabled";
    Settings::values.shaders_accurate_mul =
            LibRetro::FetchVariable("citra_use_acc_mul", "enabled") == "enabled";
    Settings::values.use_virtual_sd =
        LibRetro::FetchVariable("citra_use_virtual_sd", "enabled") == "enabled";
    Settings::values.is_new_3ds =
        LibRetro::FetchVariable("citra_is_new_3ds", "Old 3DS") == "New 3DS";
    Settings::values.swap_screen = LibRetro::FetchVariable("citra_swap_screen", "Top") == "Bottom";
    Settings::values.use_gdbstub =
        LibRetro::FetchVariable("citra_use_gdbstub", "disabled") == "enabled";

    // These values are a bit more hard to define, unfortunately.
    auto scaling = LibRetro::FetchVariable("citra_resolution_factor", "1x (Native)");
    auto endOfScale = scaling.find('x'); // All before 'x' in "_x ...", e.g "1x (Native)"
    if (endOfScale == std::string::npos) {
        LOG_ERROR(Frontend, "Failed to parse resolution scale!");
        Settings::values.resolution_factor = 1;
    } else {
        int scale = stoi(scaling.substr(0, endOfScale));
        Settings::values.resolution_factor = scale;
    }

    auto layout = LibRetro::FetchVariable("citra_layout_option", "Default Top-Bottom Screen");

    if (layout == "Default Top-Bottom Screen") {
        Settings::values.layout_option = Settings::LayoutOption::Default;
    } else if (layout == "Single Screen Only") {
        Settings::values.layout_option = Settings::LayoutOption::SingleScreen;
    } else if (layout == "Large Screen, Small Screen") {
        Settings::values.layout_option = Settings::LayoutOption::LargeScreen;
    } else if (layout == "Side by Side") {
        Settings::values.layout_option = Settings::LayoutOption::SideScreen;
    } else {
        LOG_ERROR(Frontend, "Unknown layout type: {}.", layout);
        Settings::values.layout_option = Settings::LayoutOption::Default;
    }

    // Custom layout
    Settings::values.custom_layout = LibRetro::FetchVariable("citra_custom_layout", "disabled") == "enabled";
    if(Settings::values.custom_layout == true) {
        float screensRatio = 0.0;
        float topLeftRatio = 0.0;
        float topTopRatio = 0.0;
        float bottomLeftRatio = 0.0;
        float bottomTopRatio = 0.0; 

        auto ratio = LibRetro::FetchVariable("custom_screen_size", "16/9 preset");
        if(ratio == "16/9 preset") {
            screensRatio = 0.735; // preset for 16/9 screens that should be fine with some Retroarch overlays
        } else if(ratio == "0 per thousand") {
            screensRatio = 0.0;
        } else {
            screensRatio = stof(ratio) / 1000.0;
        }

        ratio = LibRetro::FetchVariable("custom_top_top", "16/9 preset");
        if(ratio == "16/9 preset") {
            topTopRatio = 0.085; // preset for 16/9 screens that should be fine with some Retroarch overlays
        } else if(ratio == "0 per thousand") {
            topTopRatio = 0.0;
        } else {
            topTopRatio = stof(ratio) / 1000.0;
        }

        ratio = LibRetro::FetchVariable("custom_top_left", "16/9 preset");
        if(ratio == "16/9 preset") {
            topLeftRatio = 0.132; // preset for 16/9 screens that should be fine with some Retroarch overlays
        } else if(ratio == "0 per thousand") {
            topLeftRatio = 0.0;
        } else {
            topLeftRatio = stof(ratio) / 1000.0;
        }

        ratio = LibRetro::FetchVariable("custom_bottom_top", "16/9 preset");
        if(ratio == "16/9 preset") {
            bottomTopRatio = 0.560; // preset for 16/9 screens that should be fine with some Retroarch overlays
        } else if(ratio == "0 per thousand") {
            bottomTopRatio = 0.0;
        } else {
            bottomTopRatio = stof(ratio) / 1000.0;
        }

        ratio = LibRetro::FetchVariable("custom_bottom_left", "16/9 preset");
        if(ratio == "16/9 preset") {
            bottomLeftRatio = 0.206; // preset for 16/9 screens that should be fine with some Retroarch overlays
        } else if(ratio == "0 per thousand") {
            bottomLeftRatio = 0.0;
        } else {
            bottomLeftRatio = stof(ratio) / 1000.0;
        }

        // Update the custom layout values
        emu_instance->emu_window->SetCustomLayoutRatios(screensRatio, topLeftRatio, topTopRatio, bottomLeftRatio, bottomTopRatio);
    }

    auto deadzone = LibRetro::FetchVariable("citra_deadzone", "15");
    LibRetro::settings.deadzone = (float)std::stoi(deadzone) / 100;

    auto analog_function =
        LibRetro::FetchVariable("citra_analog_function", "C-Stick and Touchscreen Pointer");

    if (analog_function == "C-Stick and Touchscreen Pointer") {
        LibRetro::settings.analog_function = LibRetro::CStickFunction::Both;
    } else if (analog_function == "C-Stick") {
        LibRetro::settings.analog_function = LibRetro::CStickFunction::CStick;
    } else if (analog_function == "Touchscreen Pointer") {
        LibRetro::settings.analog_function = LibRetro::CStickFunction::Touchscreen;
    } else {
        LOG_ERROR(Frontend, "Unknown right analog function: {}.", analog_function);
        LibRetro::settings.analog_function = LibRetro::CStickFunction::Both;
    }

    auto region = LibRetro::FetchVariable("citra_region_value", "Auto");
    std::map<std::string, int> region_values;
    region_values["Auto"] = -1;
    region_values["Japan"] = 0;
    region_values["USA"] = 1;
    region_values["Europe"] = 2;
    region_values["Australia"] = 3;
    region_values["China"] = 4;
    region_values["Korea"] = 5;
    region_values["Taiwan"] = 6;

    auto result = region_values.find(region);
    if (result == region_values.end()) {
        LOG_ERROR(Frontend, "Invalid region: {}.", region);
        Settings::values.region_value = -1;
    } else {
        Settings::values.region_value = result->second;
    }

    Settings::values.touch_device = "engine:emu_window";

    // Hardcode buttons to bind to libretro - it is entirely redundant to have
    //  two methods of rebinding controls.
    // Citra: A = RETRO_DEVICE_ID_JOYPAD_A (8)
    Settings::values.buttons[0] = "button:8,joystick:0,engine:libretro";
    // Citra: B = RETRO_DEVICE_ID_JOYPAD_B (0)
    Settings::values.buttons[1] = "button:0,joystick:0,engine:libretro";
    // Citra: X = RETRO_DEVICE_ID_JOYPAD_X (9)
    Settings::values.buttons[2] = "button:9,joystick:0,engine:libretro";
    // Citra: Y = RETRO_DEVICE_ID_JOYPAD_Y (1)
    Settings::values.buttons[3] = "button:1,joystick:0,engine:libretro";
    // Citra: UP = RETRO_DEVICE_ID_JOYPAD_UP (4)
    Settings::values.buttons[4] = "button:4,joystick:0,engine:libretro";
    // Citra: DOWN = RETRO_DEVICE_ID_JOYPAD_DOWN (5)
    Settings::values.buttons[5] = "button:5,joystick:0,engine:libretro";
    // Citra: LEFT = RETRO_DEVICE_ID_JOYPAD_LEFT (6)
    Settings::values.buttons[6] = "button:6,joystick:0,engine:libretro";
    // Citra: RIGHT = RETRO_DEVICE_ID_JOYPAD_RIGHT (7)
    Settings::values.buttons[7] = "button:7,joystick:0,engine:libretro";
    // Citra: L = RETRO_DEVICE_ID_JOYPAD_L (10)
    Settings::values.buttons[8] = "button:10,joystick:0,engine:libretro";
    // Citra: R = RETRO_DEVICE_ID_JOYPAD_R (11)
    Settings::values.buttons[9] = "button:11,joystick:0,engine:libretro";
    // Citra: START = RETRO_DEVICE_ID_JOYPAD_START (3)
    Settings::values.buttons[10] = "button:3,joystick:0,engine:libretro";
    // Citra: SELECT = RETRO_DEVICE_ID_JOYPAD_SELECT (2)
    Settings::values.buttons[11] = "button:2,joystick:0,engine:libretro";
    // Citra: ZL = RETRO_DEVICE_ID_JOYPAD_L2 (12)
    Settings::values.buttons[12] = "button:12,joystick:0,engine:libretro";
    // Citra: ZR = RETRO_DEVICE_ID_JOYPAD_R2 (13)
    Settings::values.buttons[13] = "button:13,joystick:0,engine:libretro";
    // Citra: HOME = RETRO_DEVICE_ID_JOYPAD_L3 (as per above bindings) (14)
    Settings::values.buttons[14] = "button:14,joystick:0,engine:libretro";

    // Circle Pad
    Settings::values.analogs[0] = "axis:0,joystick:0,engine:libretro";
    // C-Stick
    if (LibRetro::settings.analog_function != LibRetro::CStickFunction::Touchscreen) {
        Settings::values.analogs[1] = "axis:1,joystick:0,engine:libretro";
    } else {
        Settings::values.analogs[1] = "";
    }

    // Configure the file storage location
    auto use_libretro_saves = LibRetro::FetchVariable("citra_use_libretro_save_path",
                                                      "LibRetro Default") == "LibRetro Default";

    if (use_libretro_saves) {
        auto target_dir = LibRetro::GetSaveDir();
        if (target_dir.empty()) {
            LOG_INFO(Frontend, "No save dir provided; trying system dir...");
            target_dir = LibRetro::GetSystemDir();
        }

        if (!target_dir.empty()) {
            target_dir += "/Citra";

            // Ensure that this new dir exists
            if (!FileUtil::CreateDir(target_dir)) {
                LOG_ERROR(Frontend, "Failed to create \"{}\". Using Citra's default paths.", target_dir);
            } else {
                FileUtil::GetUserPath(FileUtil::UserPath::RootDir, target_dir);
                const auto& target_dir_result = FileUtil::GetUserPath(FileUtil::UserPath::UserDir, target_dir);
                LOG_INFO(Frontend, "User dir set to \"{}\".", target_dir_result);
            }
        }
    }

    // Update the framebuffer sizing.
    emu_instance->emu_window->UpdateLayout();

    Settings::Apply();
}

/**
 * libretro callback; Called every game tick.
 */
void retro_run() {
    // Check to see if we actually have any config updates to process.
    if (LibRetro::HasUpdatedConfig()) {
        UpdateSettings();
    }

    // We can't assume that the frontend has been nice and preserved all OpenGL settings. Reset.
    auto last_state = OpenGLState::GetCurState();
    ResetGLState();
    last_state.Apply();

    while (!emu_instance->emu_window->HasSubmittedFrame()) {
        auto result = Core::System::GetInstance().RunLoop();

        if (result != Core::System::ResultStatus::Success) {
            std::string errorContent = Core::System::GetInstance().GetStatusDetails();
            std::string msg;

            switch (result) {
            case Core::System::ResultStatus::ErrorSystemFiles:
                msg = "Citra was unable to locate a 3DS system archive: " + errorContent;
                break;
            default:
                msg = "Fatal Error encountered: " + errorContent;
                break;
            }

            LibRetro::DisplayMessage(msg.c_str());
        }
    }
}

void* load_opengl_func(const char* name) {
    return (void*)emu_instance->hw_render.get_proc_address(name);
}

void context_reset() {
    if (!Core::System::GetInstance().IsPoweredOn()) {
        LOG_CRITICAL(Frontend, "Cannot reset system core if isn't on!");
        return;
    }

    // Check to see if the frontend provides us with OpenGL symbols
    if (emu_instance->hw_render.get_proc_address != nullptr) {
        if (!gladLoadGLLoader((GLADloadproc)load_opengl_func)) {
            LOG_CRITICAL(Frontend, "Glad failed to load (frontend-provided symbols)!");
            return;
        }
    } else {
        // Else, try to load them on our own
        if (!gladLoadGL()) {
            LOG_CRITICAL(Frontend, "Glad failed to load (internal symbols)!");
            return;
        }
    }

    // Recreate our renderer, so it can reset it's state.
    if (VideoCore::g_renderer != nullptr) {
        LOG_ERROR(Frontend,
                  "Likely memory leak: context_destroy() was not called before context_reset()!");
    }

    VideoCore::g_renderer = std::make_unique<RendererOpenGL>(*emu_instance->emu_window);
    if (VideoCore::g_renderer->Init() != Core::System::ResultStatus::Success) {
        LOG_DEBUG(Render, "initialized OK");
    } else {
        LOG_ERROR(Render, "initialization failed!");
    }

    emu_instance->emu_window->UpdateLayout();
    emu_instance->emu_window->CreateContext();
}

void context_destroy() {
    if (VideoCore::g_renderer != nullptr) {
        VideoCore::g_renderer->ShutDown();
    }

    emu_instance->emu_window->DestroyContext();
    VideoCore::g_renderer = nullptr;
}

void retro_reset() {
    Core::System::GetInstance().Shutdown();
    Core::System::GetInstance().Load(*emu_instance->emu_window, LibRetro::settings.file_path);
    context_reset(); // Force the renderer to appear
}

/**
 * libretro callback; Called when a game is to be loaded.
 */
bool retro_load_game(const struct retro_game_info* info) {
    LOG_INFO(Frontend, "Starting Citra RetroArch game...");

    LibRetro::settings.file_path = info->path;

    LibRetro::SetHWSharedContext();

    if (!LibRetro::SetPixelFormat(RETRO_PIXEL_FORMAT_XRGB8888)) {
        LOG_CRITICAL(Frontend, "XRGB8888 is not supported.");
        LibRetro::DisplayMessage("XRGB8888 is not supported.");
        return false;
    }

    emu_instance->hw_render.context_type = RETRO_HW_CONTEXT_OPENGL_CORE;
    emu_instance->hw_render.version_major = 3;
    emu_instance->hw_render.version_minor = 3;
    emu_instance->hw_render.context_reset = context_reset;
    emu_instance->hw_render.context_destroy = context_destroy;
    emu_instance->hw_render.cache_context = false;
    emu_instance->hw_render.bottom_left_origin = true;
    if (!LibRetro::SetHWRenderer(&emu_instance->hw_render)) {
        LOG_CRITICAL(Frontend, "OpenGL 3.3 is not supported.");
        LibRetro::DisplayMessage("OpenGL 3.3 is not supported.");
        return false;
    }

    emu_instance->emu_window = std::make_unique<EmuWindow_LibRetro>();
    UpdateSettings();

    const Core::System::ResultStatus load_result{Core::System::GetInstance().Load(
        *emu_instance->emu_window, LibRetro::settings.file_path)};

    switch (load_result) {
    case Core::System::ResultStatus::ErrorGetLoader:
        LOG_CRITICAL(Frontend, "Failed to obtain loader for {}!",
                     LibRetro::settings.file_path);
        LibRetro::DisplayMessage("Failed to obtain loader for specified ROM!");
        return false;
    case Core::System::ResultStatus::ErrorLoader:
        LOG_CRITICAL(Frontend, "Failed to load ROM!");
        LibRetro::DisplayMessage("Failed to load ROM!");
        return false;
    case Core::System::ResultStatus::ErrorLoader_ErrorEncrypted:
        LOG_CRITICAL(Frontend, "The game that you are trying to load must be decrypted before "
                               "being used with Citra. \n\n For more information on dumping and "
                               "decrypting games, please refer to: "
                               "https://citra-emu.org/wiki/Dumping-Game-Cartridges");
        LibRetro::DisplayMessage("The game that you are trying to load must be decrypted before "
                                 "being used with Citra. \n\n For more information on dumping and "
                                 "decrypting games, please refer to: "
                                 "https://citra-emu.org/wiki/Dumping-Game-Cartridges");
        return false;
    case Core::System::ResultStatus::ErrorLoader_ErrorInvalidFormat:
        LOG_CRITICAL(Frontend, "Error while loading ROM: The ROM format is not supported.");
        LibRetro::DisplayMessage("Error while loading ROM: The ROM format is not supported.");
        return false;
    case Core::System::ResultStatus::ErrorNotInitialized:
        LOG_CRITICAL(Frontend, "CPUCore not initialized");
        LibRetro::DisplayMessage("CPUCore not initialized");
        return false;
    case Core::System::ResultStatus::ErrorSystemMode:
        LOG_CRITICAL(Frontend, "Failed to determine system mode!");
        LibRetro::DisplayMessage("Failed to determine system mode!");
        return false;
    case Core::System::ResultStatus::ErrorVideoCore:
        LOG_CRITICAL(Frontend, "VideoCore not initialized");
        LibRetro::DisplayMessage("VideoCore not initialized");
        return false;
    case Core::System::ResultStatus::Success:
        break; // Expected case
    default:
        LOG_CRITICAL(Frontend, "Unknown error");
        LibRetro::DisplayMessage("Unknown error");
        return false;
    }

    return true;
}

void retro_unload_game() {
    LOG_DEBUG(Frontend, "Unloading game...");
    Core::System::GetInstance().Shutdown();
}

unsigned retro_get_region() {
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info,
                             size_t num_info) {
    return retro_load_game(info);
}

size_t retro_serialize_size() {
    return 0;
}

bool retro_serialize(void* data_, size_t size) {
    return true;
}

bool retro_unserialize(const void* data_, size_t size) {
    return true;
}

void* retro_get_memory_data(unsigned id) {
    if ( id == RETRO_MEMORY_SYSTEM_RAM )
        return Kernel::memory_regions[0].linear_heap_memory->data();

    return NULL;
}

size_t retro_get_memory_size(unsigned id) {
    if ( id == RETRO_MEMORY_SYSTEM_RAM )
        return Kernel::memory_regions[0].size;

    return 0;
}

void retro_cheat_reset() {}

void retro_cheat_set(unsigned index, bool enabled, const char* code) {}
