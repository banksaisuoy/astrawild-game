# ASTRAWILD — Technical Architecture Specification

## 1. เป้าหมายทางเทคนิค

สถาปัตยกรรมของ ASTRAWILD จะเน้นการแยกความรับผิดชอบให้ชัดเจน เพื่อให้ทีมสามารถเพิ่มอีโค พื้นที่ สูตรคราฟต์ และเควสต์ได้โดยไม่ต้องแก้โค้ดระบบแกนกลางทุกครั้ง โค้ด C++ จะรับผิดชอบกฎที่ต้องเสถียร ประสิทธิภาพ และความถูกต้องของข้อมูล ส่วน Blueprint จะใช้ประกอบคอนเทนต์ ปรับจังหวะ และสร้างต้นแบบที่ต้องเปลี่ยนบ่อย

โครงการจะใช้ **Data-Driven Gameplay** เป็นหลัก อีโค ไอเทม ความสามารถ สูตรคราฟต์ อาคาร เควสต์ และเหตุการณ์จะมี Primary Data Asset หรือ Data Table เป็นแหล่งข้อมูลกลาง ระบบเกมจะอ้างอิง ID มากกว่าการอ้างอิง Actor แบบฝังในเลเวล เพื่อให้การบันทึก การทำซ้ำ และ Multiplayer เชื่อถือได้

## 2. โครงสร้างโปรเจกต์

โครงสร้างโมดูลที่แนะนำมีดังนี้

| โมดูล | ความรับผิดชอบ | สิ่งที่ไม่ควรใส่ |
|---|---|---|
| `AstrawildCore` | อินเทอร์เฟซ ระบบ ID, Tags, เวลา, การตั้งค่า และยูทิลิตี | กฎเฉพาะของอีโคหรือ UI |
| `AstrawildGameplay` | ตัวละคร อีโค การต่อสู้ ความเสียหาย ความสามารถ และสถานะ | โค้ดเชื่อมเซิร์ฟเวอร์ภายนอก |
| `AstrawildWorld` | World Partition, จุดเกิด ทรัพยากร สภาพอากาศ และเหตุการณ์โลก | กฎอินเวนทอรีส่วนตัว |
| `AstrawildBuilding` | Grid การสร้าง สิ่งปลูกสร้าง การผลิต และพลังงาน | ระบบต่อสู้โดยตรง |
| `AstrawildProgression` | เควสต์ เทคโนโลยี สมุดภาคสนาม ความสัมพันธ์ และการปลดล็อก | การวาด UI |
| `AstrawildSave` | Schema, serialization, migration, checksum และ backup | การตัดสินใจเล่นเกม |
| `AstrawildNet` | Session, authority, replication, permissions และ RPC | กฎที่ควรอยู่ใน Gameplay |
| `AstrawildUI` | MVVM/ViewModel, menus, HUD, accessibility และ input hints | การแก้สถานะเกมโดยตรง |
| `AstrawildEditor` | เครื่องมือตรวจ Data Asset, spawn preview, world validation และ batch commandlet | โค้ดที่ต้องใช้ตอนเล่นจริง |

โฟลเดอร์ Content ควรแบ่งตามระบบและประเภทคอนเทนต์ เช่น `/Game/Astrawild/Characters/Echoes`, `/Game/Astrawild/World/Regions`, `/Game/Astrawild/Data`, `/Game/Astrawild/UI` และ `/Game/Astrawild/Audio` พร้อมกำหนด naming convention และกฎห้ามอ้างอิงข้ามชั้นแบบวนกลับ

## 3. Unreal Gameplay Framework

ระบบหลักควรแมปกับคลาสมาตรฐานของ Unreal ดังนี้

| คลาส/ระบบ | หน้าที่ใน ASTRAWILD |
|---|---|
| `GameInstance` | ตัวจัดการการเริ่มเกม การตั้งค่า และ service ระดับโปรเซส |
| `GameMode` | กฎของเซสชัน ฝั่งเซิร์ฟเวอร์เท่านั้น |
| `GameState` | สถานะโลกและเหตุการณ์ที่ต้องเผยแพร่ให้ผู้เล่น |
| `PlayerController` | อินพุต การสั่งงาน UI และการส่งคำขอไปเซิร์ฟเวอร์ |
| `PlayerState` | ID ผู้เล่น สถิติ ปาร์ตี้ และข้อมูลที่ต้อง replicate |
| `Character` | การเคลื่อนที่ แอนิเมชัน collision และความสามารถที่ใช้งานได้ |
| `AbilitySystemComponent` | ความสามารถ Attribute Effects Tags และการคำนวณความเสียหาย |
| `ActorComponent` | อินเวนทอรี ความสัมพันธ์ การโต้ตอบ และงานฐานแบบแยกโมดูล |
| `WorldSubsystem` | ระบบสภาพอากาศ จุดเกิด ทรัพยากร และบริการของโลก |
| `GameInstanceSubsystem` | Save, session, analytics, audio และ configuration |
| `DataAsset/DataTable` | ค่าคอนเทนต์ที่ออกแบบและปรับสมดุลได้โดยไม่แก้โค้ด |

การใช้ Gameplay Ability System เหมาะกับการรวมกฎของสกิล อาวุธ สถานะธาตุ บัฟ ดีบัฟ และความเสียหายให้เป็นรูปแบบเดียวกัน แต่ระบบที่ไม่ต้องการการคำนวณแบบต่อเนื่อง เช่น การเปิดกล่องหรือการเปลี่ยนสูตรคราฟต์ ไม่ควรนำไปทำเป็น Ability ทั้งหมด เพราะจะเพิ่มความซับซ้อนโดยไม่จำเป็น

## 4. รูปแบบข้อมูลแกนกลาง

ทุกสิ่งที่ผู้เล่นสะสมหรือเปลี่ยนแปลงได้ต้องมี Stable ID และ Version อีโคจะมี `EchoDefinitionId`, อินสแตนซ์ที่ผู้เล่นครอบครองจะมี `EchoInstanceId` ส่วนโลกจะมี `WorldObjectId` สำหรับฐาน กล่อง และสิ่งปลูกสร้าง ข้อมูลที่เปลี่ยนแปลงต้องไม่ผูกกับชื่อ Asset หรือ index ในอาร์เรย์ที่อาจเปลี่ยนเมื่ออัปเดตเกม

| ข้อมูล | ระดับ | ตัวอย่างฟิลด์ |
|---|---|---|
| Definition | คงที่ | ชื่อ รูปลักษณ์ ธาตุ สกิล ตารางดรอป |
| Instance | เฉพาะผู้เล่น | เลเวล ค่าความไว้ใจ บุคลิก สถานะ และประสบการณ์ |
| World State | เฉพาะโลก | ตำแหน่ง HP งานผลิต เจ้าของ และเวลาสร้าง |
| Session State | ชั่วคราว | ผู้เล่นออนไลน์ เหตุการณ์ active และการเชื่อมต่อ |
| Presentation | ภาพและเสียง | VFX SFX animation montage และ UI icon |

ระบบ Event Bus จะใช้สำหรับการสื่อสารแบบหลวม เช่น `OnEchoCaptured`, `OnQuestUpdated`, `OnWorldEventStarted` แต่การแก้ข้อมูลสำคัญต้องผ่าน service หรือ command ที่ตรวจสอบได้ ไม่ให้ทุก Actor เปลี่ยนข้อมูลของกันและกันโดยตรง

## 5. โลกกว้างและการสตรีม

แผนที่หลักจะใช้ World Partition เพื่อเก็บโลกใน persistent level เดียวและแบ่งเป็น grid cell ที่โหลดและปลดโหลดตามระยะจาก streaming source ระบบนี้ออกแบบมาเพื่อจัดการโลกขนาดใหญ่และทำงานร่วมกับ One File Per Actor, Data Layers, Level Instancing และ HLOD [1]

แนวทางการตั้งค่ารุ่นแรกคือใช้ runtime grid เดียวเป็นหลัก แบ่ง cell ขนาดกลางและกำหนด loading range จากการทดสอบจริง ไม่ใช้ grid จำนวนมากโดยไม่มีเหตุผล เพราะการจัดการหลาย grid เพิ่มภาระการสตรีมและการดีบัก แต่ละเขตใช้ Data Layer สำหรับสถานะกลางวัน/กลางคืน เหตุการณ์เควสต์ และความเสียหายของพื้นที่

HLOD ใช้สำหรับทิวทัศน์และสิ่งก่อสร้างระยะไกล ส่วนวัตถุที่ผู้เล่นโต้ตอบต้องถูกโหลดเฉพาะเมื่ออยู่ในระยะที่เหมาะสม การวาร์ปหรือเดินทางไปพื้นที่ใหม่ต้องสร้าง temporary streaming source ล่วงหน้าเพื่อให้เซลล์ปลายทางโหลดก่อนย้ายผู้เล่น ระบบ minimap ต้องสร้างจาก world data ไม่ใช่ render แผนที่ทั้งโลกแบบ runtime

## 6. AI และการจำลองอีโค

อีโคที่อยู่ใกล้ผู้เล่นใช้ Character หรือ Pawn พร้อม StateTree/Behavior Tree สำหรับพฤติกรรมที่ต้องการความแม่นยำ เช่น ต่อสู้ หนี กินอาหาร เล่นกับฝูง และทำงานฐาน อีโคที่อยู่ไกลใช้ระบบจำลองราคาถูกระดับ state เช่น เดินทาง หาพื้นที่อาหาร พัก หรือย้ายฝูง โดยไม่ spawn skeletal actor เต็มรูปแบบตลอดเวลา

Mass Entity เหมาะสำหรับการคำนวณแบบ data-oriented และการจำลองเอนทิตีจำนวนมาก จึงสามารถนำมาพิจารณาสำหรับฝูงอีโคระยะไกลหรือสัตว์พื้นหลัง แต่ไม่ควรย้ายตัวละครที่ผู้เล่นกำลังจับ ควบคุม หรือกำลังต่อสู้ไปใช้ระบบที่ทีมยังไม่ชำนาญทั้งหมดในครั้งเดียว [2]

| ระยะจากผู้เล่น | รูปแบบการจำลอง | สิ่งที่ต้องคงไว้ |
|---|---|---|
| 0–40 เมตร | Actor เต็มรูปแบบ AI, animation, collision และ interaction | ตำแหน่ง การตัดสินใจ และสถานะที่ผู้เล่นมองเห็น |
| 40–150 เมตร | Actor ลดความถี่ tick, animation budget และ collision | พฤติกรรมสำคัญ เหตุการณ์โจมตี และทิศทาง |
| มากกว่า 150 เมตร | Lightweight simulation หรือ Mass representation | สถานะฝูง พลังชีวิตโดยประมาณ และเส้นทาง |
| เซลล์ที่ไม่โหลด | เก็บเป็น persistent world state | เวลา การย้ายพื้นที่ การเกิดใหม่ และทรัพยากร |

ระบบ AI ต้องมี debug mode ที่แสดง perception, current state, target, path, task และเหตุผลที่หยุดทำงาน รวมถึงมี test map สำหรับทดสอบฝูง อากาศ และการเปลี่ยนระดับความละเอียดโดยไม่ต้องเล่นผ่านทั้งเกม

## 7. อินเวนทอรี คราฟต์ และฐาน

อินเวนทอรีใช้ `InventoryComponent` ที่ถือรายการ `ItemStack` โดยแยก Item Definition ออกจากจำนวนและ metadata ของ stack สูตรคราฟต์ใช้ `RecipeDefinition` และตรวจสอบวัตถุดิบผ่าน service เดียวทั้งใน Single-player และ Server เพื่อไม่ให้เกิดความแตกต่างของผลลัพธ์

ระบบสร้างฐานใช้ grid ที่คำนวณตำแหน่งฝั่ง client เพื่อให้ผู้เล่นเห็น preview ทันที แต่การยืนยันการวางต้องตรวจสอบฝั่ง server ได้แก่ ระยะจากฐาน สิทธิ์เจ้าของ การชน พื้นที่ที่อนุญาต วัตถุดิบ และข้อจำกัดของเขต สิ่งปลูกสร้างที่สร้างเสร็จจะกลายเป็น world object ที่มี ID คงที่และบันทึกเฉพาะ delta ไม่บันทึกข้อมูลของฉากทั้งก้อน

## 8. การบันทึกข้อมูล

Save subsystem แบ่งข้อมูลเป็นสามชุด ได้แก่ `PlayerProfile`, `WorldSnapshot` และ `SettingsProfile` โดยใช้ schema version และ migration chain เช่น `v1 -> v2 -> v3` การบันทึกอัตโนมัติทำผ่าน queue ที่รวมการเปลี่ยนแปลงหลายรายการและเขียนบนช่วงปลอดภัย ไม่ทำ synchronous write ทุกครั้งที่ผู้เล่นเก็บไอเทมหนึ่งชิ้น

การบันทึกโลกใช้ snapshot เป็นระยะร่วมกับ event log ขนาดเล็ก เพื่อให้กู้คืนได้หากเกมปิดผิดปกติ ก่อนเขียนไฟล์ใหม่ต้องเขียน temporary file และเปลี่ยนชื่อแบบ atomic พร้อม checksum ตัวละครและอีโคที่ผู้เล่นเป็นเจ้าของต้องมี backup แยก เพื่อป้องกันไฟล์โลกเสียหายแล้วลากข้อมูลผู้เล่นเสียหายไปด้วย

## 9. Multiplayer และความปลอดภัย

ระบบ Co-op ใช้ server authority สำหรับการสร้างไอเทม การเก็บทรัพยากร การจับอีโค การทำดาเมจ การสร้างฐาน และการเปลี่ยนสถานะเควสต์ Client ส่ง intent เช่น “ขอเก็บไอเทม” หรือ “ขอใช้สกิล” แล้ว server ตรวจสอบระยะ cooldown สิทธิ์ และสถานะก่อน broadcast ผลลัพธ์

ในช่วงต้นใช้ระบบ replication มาตรฐานร่วมกับ Replication Graph หรือการ cull ตามระยะที่เหมาะสม แล้วทดสอบ Iris ใน branch แยกเมื่อมีข้อมูลจำนวน actor และ bandwidth จริง Iris เป็นระบบ opt-in ที่ Epic ระบุว่าสนับสนุนโลกที่มีปฏิสัมพันธ์มาก ผู้เล่นจำนวนมากขึ้น และต้นทุนเซิร์ฟเวอร์ต่ำลง แต่เอกสารปัจจุบันยังเตือนให้ใช้ด้วยความระมัดระวังเนื่องจากเป็น Experimental [3]

| ความเสี่ยง | วิธีป้องกัน |
|---|---|
| Client สร้างของเอง | ให้ server เป็นผู้สร้างและตรวจสอบ inventory |
| ยิงทะลุหรือทำดาเมจเกิน | ตรวจสอบ trajectory, cooldown และ hit result ฝั่ง server |
| ฐานหรือวัตถุซ้ำ | ใช้ command id, world object id และ idempotent transaction |
| Save ถูกย้อนแก้ | ใช้ checksum, server-side persistence และ validate schema |
| ผู้เล่นส่ง RPC ถี่เกิน | rate limit, reject policy และบันทึก telemetry |
| การเชื่อมต่อหลุด | reconnect window, transaction rollback และ save checkpoint |

## 10. กราฟิกและระบบแสงในเชิงสถาปัตยกรรม

Nanite จะใช้กับ static environment, rocks, ruins, cliffs และทรัพย์สินที่เหมาะสม แต่ทีมต้องมี fallback mesh และ LOD แบบดั้งเดิมสำหรับวัตถุที่ไม่รองรับหรือแพลตฟอร์มเป้าหมาย การใช้ Nanite ไม่ได้ลบต้นทุนของวัสดุ เงา จำนวนอินสแตนซ์ หรือความละเอียดภาพ จึงต้องตรวจด้วย Unreal Insights และเครื่องมือ Buffer Visualization ทุก milestone [4]

Lumen จะเปิดใช้ใน preset High/Ultra เพื่อให้เกิดแสงสะท้อนและ global illumination แบบไดนามิก ส่วน preset Medium จะลดระยะและคุณภาพ หรือเปลี่ยนไปใช้ระบบแสงที่ถูกกว่า หากต้องการให้เครื่องระดับกลางเล่นได้ลื่น การออกแบบฉากต้องพึ่งแสงที่จัดวางดีและ silhouette ที่ชัด ไม่ใช้แสงไดนามิกจำนวนมากเพื่อแก้ปัญหาการออกแบบ

## 11. UI และการเชื่อมระบบ

UI ใช้แนวทาง ViewModel หรือ MVVM เพื่อให้ Widget แสดงข้อมูลผ่านสถานะที่สังเกตได้ ไม่ให้ Widget เรียกแก้ GameState โดยตรง หน้าหลักประกอบด้วย HUD, interaction prompt, party, quick wheel, inventory, field journal, map, quest log, base management และ settings

การเปิดเมนูต้องมี input context แยกจากการเล่น เมนูสำคัญรองรับ mouse, keyboard, controller และระบบนำทางด้วย focus ทุกจอมีสถานะ loading, empty, error และ reconnect ไม่แสดงหน้าว่างที่ทำให้ผู้เล่นไม่รู้ว่าเกมกำลังทำงานหรือหยุดอยู่

## 12. การจัดการ Asset และการทำงานเป็นทีม

ทีมใช้ source control ที่ล็อกไฟล์ binary ของ Unreal อย่างเหมาะสม พร้อม One File Per Actor และกฎไม่ให้หลายคนแก้ Level เดียวกันโดยไม่แบ่งพื้นที่ การสร้าง asset ต้องมี metadata ได้แก่ owner, status, target platform, memory budget และ LOD policy ทุก asset ที่เข้าเกมต้องผ่าน validation อัตโนมัติ เช่น ชื่อ วัสดุ collision texture size Nanite flag และการอ้างอิงที่เสีย

Derived Data Cache แบบ shared ควรนำมาใช้เมื่อทีมมี asset จำนวนมาก เพื่อช่วยลดเวลาการนำเข้าและ build ของ Nanite และ shader การ build ที่ใช้เวลานานต้องทำแบบ incremental และมี CI สำหรับ compile, cook, automated test และ package nightly

## 13. แผนการตัดสินใจทางเทคนิค

ทีมจะไม่ตัดสินใจจากชื่อฟีเจอร์หรือภาพตัวอย่าง แต่ใช้การทดลองขนาดเล็กและตัวเลขจาก target hardware ทุกระบบที่มีความเสี่ยงสูงต้องมี spike project แยก เช่น การโหลด World Partition ขณะวิ่งเร็ว การสลับอีโคพร้อมกันสี่คน ฝูง AI นอกจอ การสร้างฐานจำนวนมาก และการ reconnect พร้อม save

| การทดลอง | เกณฑ์ผ่านเบื้องต้น |
|---|---|
| เดินทางผ่านสองเซลล์ | ไม่มี hitch ที่ทำให้การควบคุมสะดุดในเครื่องเป้าหมาย |
| ต่อสู้กับอีโค 12 ตัว | frame time และ AI tick ไม่เกินงบที่กำหนด |
| ผู้เล่น 4 คนสร้างฐานเดียวกัน | ไม่มี desync ของสิ่งปลูกสร้างและ inventory |
| Save ระหว่างเหตุการณ์ | โหลดกลับมาแล้วไม่มีของหายหรือซ้ำ |
| ปิด Lumen/Nanite บางส่วน | ภาพยังอ่านง่ายและ gameplay ไม่พัง |
| ฝูงอีโค 100+ ตัว | ใช้ simulation แบบลดระดับได้โดยไม่ spawn Actor ทั้งหมด |

## References

[1]: https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine "Epic Games — World Partition in Unreal Engine"

[2]: https://dev.epicgames.com/documentation/en-us/unreal-engine/mass-entity-in-unreal-engine "Epic Games — Mass Entity in Unreal Engine"

[3]: https://dev.epicgames.com/documentation/en-us/unreal-engine/iris-replication-system-in-unreal-engine "Epic Games — Iris Replication System in Unreal Engine"

[4]: https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine "Epic Games — Nanite Virtualized Geometry Overview"

[5]: https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-global-illumination-and-reflections-in-unreal-engine "Epic Games — Lumen Global Illumination and Reflections"
