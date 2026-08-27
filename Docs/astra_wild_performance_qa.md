# ASTRAWILD — Performance, Security & QA Plan

## 1. เป้าหมายประสิทธิภาพ

เกมจะออกแบบโดยยึด target hardware เป็นข้อกำหนดตั้งแต่วันแรก ไม่ใช่รอ optimize ตอนใกล้เปิดตัว เป้าหมายหลักคือ **60 FPS บนเครื่องระดับกลางในฉากเล่นทั่วไป** และต้องรักษาการควบคุมให้ตอบสนองได้ดีแม้ช่วงต่อสู้ มีฝูงอีโค หรือกำลังโหลดพื้นที่ใหม่ ฉากที่หนักเป็นพิเศษอาจลดลงถึง 30 FPS ได้ชั่วคราว แต่ต้องไม่เกิดอาการกระตุกยาวหรือ input หาย

การวัดจะใช้ frame time มากกว่า FPS เพียงตัวเดียว เพราะ frame time บอกได้ว่าปัญหาอยู่ที่ Game Thread, Render Thread, GPU, loading หรือการกระตุกจาก shader/asset streaming โดยตั้งงบเริ่มต้น 16.67 ms ต่อเฟรมสำหรับ 60 FPS แล้วแบ่งให้ระบบต่าง ๆ เป็นงบตรวจสอบ ไม่ใช่ตัวเลขตายตัวที่ห้ามปรับเมื่อข้อมูลจริงชี้ว่าควรแบ่งใหม่

| งบตรวจสอบต่อเฟรมบน target PC | เป้าหมายเริ่มต้น | สิ่งที่ต้องติดตาม |
|---|---:|---|
| Game Thread | ≤ 6.0 ms | Actor tick, AI, gameplay, physics และ blueprint execution |
| Render Thread | ≤ 3.5 ms | draw submission, scene proxy, visibility และ UI |
| GPU | ≤ 12.0 ms | lighting, shadows, material, post process และ effects |
| Loading/streaming hitch | ไม่มี hitch รุนแรงระหว่างการเดินทางปกติ | World Partition, texture, shader และ animation streaming |
| Memory | มี headroom อย่างน้อย 15% จากเพดานแพลตฟอร์ม | texture, Nanite, audio, save, streaming และ fragmentation |
| Network simulation | tick และ bandwidth ต้องคงที่เมื่อมี 4 คน | replication, RPC, relevancy และ packet loss |

## 2. Target hardware และกราฟิก preset

การกำหนดสเปกต้องใช้เครื่องจริงหลายระดับ ไม่ควรใช้เพียงคอมพิวเตอร์ของทีมพัฒนาเป็นตัวแทน ผู้เล่นกลุ่มหลักจะอยู่ที่ Medium preset ส่วน Low ต้องยังอ่านพื้นที่อันตราย จุดโต้ตอบ และสถานะการต่อสู้ได้ครบ แม้ลดคุณภาพแสง เงา พืช และเอฟเฟกต์ลง

| โปรไฟล์ | ลักษณะเครื่อง | เป้าหมาย |
|---|---|---|
| Minimum | CPU 4–6 cores, RAM 16 GB, GPU VRAM 6 GB, SSD | เล่นได้ที่ 30 FPS ด้วย Low และระยะมองเห็นเหมาะสม |
| Target | CPU 6–8 cores, RAM 16–32 GB, GPU VRAM 8 GB, SSD | 60 FPS ใน Medium/High ตามฉาก |
| Recommended | CPU 8 cores ขึ้นไป, RAM 32 GB, GPU VRAM 10–12 GB, SSD | 60 FPS High พร้อม Lumen ในฉากส่วนใหญ่ |
| High-end | GPU รุ่นสูงและจอความละเอียดสูง | เพิ่มคุณภาพภาพ ไม่ใช้เป็นเกณฑ์พื้นฐานของการออกแบบ |

ตัวเลขนี้เป็นเป้าหมายวางแผนและต้องยืนยันด้วยการทดสอบเมื่อเลือกแพลตฟอร์มและ asset จริงแล้ว หากภายหลังมีเป้าหมายคอนโซลหรือมือถือ จะต้องสร้าง performance profile และ memory budget แยก ไม่ใช้ค่าของ PC ไปคาดเดาแทน

## 3. วิธีทำให้เกมลื่น

สิ่งที่สำคัญที่สุดคือการลดงานที่ไม่จำเป็นก่อนเพิ่มเทคนิคขั้นสูง ตัวละครที่อยู่ไกลควรลดความถี่ AI และ animation ไม่ใช่คำนวณเหมือนตัวที่อยู่หน้า player ทุกตัว พืชจำนวนมากควรใช้ instancing และ culling ที่วัดผลได้ สิ่งปลูกสร้างควรเปิดใช้งาน collision และ tick เฉพาะที่จำเป็น และระบบ event ควรเปลี่ยนเป็น event-driven เมื่อไม่ต้องตรวจทุกเฟรม

World Partition จะโหลดเซลล์ตาม streaming source และสามารถใช้ HLOD สำหรับฉากไกลได้ [1] แต่ทีมต้องกำหนดขนาด cell, loading range, priority และ HLOD layer จากการทดลองเดินทางจริง เมื่อ player วิ่งหรือใช้พาหนะเร็ว ระบบต้อง preload พื้นที่ด้านหน้าและปลดของด้านหลังโดยไม่ทำให้ memory เต็ม

Nanite ช่วยจัดการ geometry และ streaming รายละเอียดของ mesh ได้มาก แต่ไม่ได้ทำให้ material, lighting, shadow, foliage หรือ texture ฟรีทั้งหมด [2] ดังนั้น asset validation ต้องรายงานขนาด texture, จำนวน material slot, Nanite status, fallback mesh, shadow mode และจำนวน instance ก่อนเข้า main branch

Lumen ให้ global illumination และ reflection แบบไดนามิก แต่ค่าใช้จ่ายจะเปลี่ยนตามคุณภาพและระยะที่รักษาฉากไว้ [3] จึงต้องมี rendering policy ตาม preset ได้แก่ High/Ultra ใช้ Lumen มากขึ้น, Medium ลด quality และ view distance, Low ใช้ทางเลือกแสงที่ถูกกว่าและปิด reflection ที่ไม่จำเป็น

## 4. เครื่องมือ profiling และ checkpoint

ทุก milestone ต้องมี performance capture ที่ทำซ้ำได้ ได้แก่เส้นทางเดินทางมาตรฐาน ฉากต่อสู้มาตรฐาน การโหลดฐานใหญ่ การวาร์ป การเริ่มเซสชัน Co-op และการเปิดเมนูหนัก การ capture ต้องเก็บ build number, platform, resolution, preset, driver, map, player count และ seed/สถานะโลก เพื่อเปรียบเทียบระหว่าง build ได้

| เครื่องมือ/วิธีตรวจ | ใช้ตอบคำถาม |
|---|---|
| Unreal Insights | Thread ไหนใช้เวลา และเกิด hitch ที่จุดใด |
| Stat Unit / Stat GPU | ปัญหาอยู่ที่ Game, Draw หรือ GPU |
| MemReport / Memory Insights | memory ถูกใช้กับ asset หรือ subsystem ใด |
| RenderDoc/งบ render | pass ใดมีต้นทุนสูงและวัสดุใดซ้ำซ้อน |
| Network profiling | actor/RPC ใดใช้ bandwidth มาก |
| Automated load test | เกมยังเสถียรเมื่อมี AI และผู้เล่นตามจำนวนเป้าหมายหรือไม่ |
| Hardware capture | ผลบนเครื่องจริงต่างจาก Editor หรือไม่ |

เกณฑ์ performance regression คือเมื่อ build ใหม่ทำให้ frame time แย่ลงเกิน threshold ที่กำหนดในฉากมาตรฐาน จะเปิด issue และต้องมีคำอธิบายว่าคุ้มค่ากับฟีเจอร์ใหม่หรือไม่ การบอกว่า “เครื่องทีมยังเล่นได้” ไม่ถือเป็นหลักฐานผ่าน

## 5. ความปลอดภัยของเกมและ Multiplayer

ทุกคำขอจาก client ต้องถือว่าไม่น่าเชื่อถือ server จะตรวจสอบระยะ ตำแหน่ง สิทธิ์ cooldown วัตถุดิบ สถานะเควสต์ และความถูกต้องของเป้าหมายก่อนทำธุรกรรม การส่งค่า damage, item result, capture success หรือ build result จาก client โดยตรงเป็นข้อห้าม

Iris เป็นระบบ opt-in และเอกสารทางการระบุสถานะ Experimental ในเอกสารรุ่นปัจจุบัน จึงควรทดสอบใน branch แยกและไม่ผูกแผนเปิดตัวไว้กับระบบนี้จนกว่าจะมีผลการทดสอบจริง [4] ระบบ replication ที่ใช้ต้องกำหนด relevancy, dormancy และ update frequency ตามระยะและความสำคัญ ไม่ replicate ออบเจ็กต์ทุกอย่างให้ผู้เล่นทุกคน

| พื้นที่เสี่ยง | มาตรการป้องกัน | การตรวจ |
|---|---|---|
| สร้างไอเทม/เงินซ้ำ | server transaction และ idempotency key | replay command และ reconnect test |
| แก้ค่าตัวละคร | server-side profile, checksum และ validation | tampered save / invalid payload |
| RPC spam | rate limit, cooldown และ disconnect policy | flood test |
| ตำแหน่งผิดปกติ | server movement sanity check และ teleport rule | speed/teleport simulation |
| ดาเมจโกง | server hit validation และ ability authority | latency + impossible hit test |
| ฐานชนกัน | server placement validation และ object ID | concurrent build test |
| แชต/ชื่อผู้เล่น | filter, report, mute และ moderation log | abuse scenario test |

ระบบป้องกันต้องไม่ทำให้ผู้เล่นปกติที่ latency สูงถูกลงโทษโดยไม่จำเป็น จึงควรใช้ tolerance ที่วัดได้และแจ้งเหตุผลเมื่อปฏิเสธคำขอ การตรวจสอบฝั่ง server สำคัญกว่าการซ่อน UI หรือทำให้ client ส่งข้อมูลที่ดูปลอดภัย

## 6. แผนการทดสอบ

QA จะเริ่มตั้งแต่ Vertical Slice โดยแบ่งเป็น unit test, functional test, integration test, performance test, network test, compatibility test, accessibility test และ playtest กับผู้เล่นจริง การทดสอบต้องมีทั้งกรณีปกติและกรณีที่ผู้เล่นทำสิ่งแปลก เช่น ปิดเกมระหว่างสร้างฐาน หลุดตอนจับอีโค วางของติดขอบเซลล์ หรือเปิดเมนูขณะกำลังถูกโจมตี

| ประเภท | ขอบเขต | เกณฑ์ผ่าน |
|---|---|---|
| Unit | สูตรดาเมจ inventory save migration และ validation | ผลลัพธ์ถูกต้องและครอบคลุม edge case |
| Functional | การเคลื่อนที่ จับ คราฟต์ ฐาน เควสต์ UI | ทำตาม acceptance criteria ได้ครบ |
| Integration | Gameplay + Save + World + Network | ข้อมูลไม่หายและสถานะสอดคล้อง |
| Performance | ฉากมาตรฐานและเครื่องจริง | อยู่ใน budget ต่อเนื่อง ไม่ใช่เฉลี่ยช่วงสั้น |
| Network | 1–4 คน latency และ packet loss | ไม่มี dupes/desync ที่ทำลายความคืบหน้า |
| Compatibility | GPU, resolution, controller และ OS | เปิดเกม ตั้งค่า และเล่น core loop ได้ |
| Accessibility | subtitle, remap, colorblind, camera | ผู้เล่นเป้าหมายทำภารกิจหลักได้ |
| Playtest | ผู้เล่นใหม่และผู้เล่นซ้ำ | เข้าใจเป้าหมาย รู้สึกสนุก และไม่ติด blocker |

## 7. Crash, telemetry และการดูแลหลังเปิดตัว

เกมต้องเก็บ crash dump และ log ที่ตัดข้อมูลส่วนตัวที่ไม่จำเป็น พร้อม build ID, map, hardware profile, last checkpoint และ subsystem ที่ทำงานก่อนเกิดปัญหา Telemetry ต้องใช้เพื่อดู performance, matchmaking, session drop, quest abandon, save failure และข้อผิดพลาดซ้ำ ไม่ใช้ติดตามข้อมูลส่วนตัวเกินความจำเป็น

ระบบ log แบ่งระดับ Verbose, Log, Warning และ Error เพื่อไม่ให้เขียนข้อมูลจำนวนมากใน shipping build การแจ้งปัญหาจากผู้เล่นควรรวบรวม save slot ID แบบไม่เปิดเผยข้อมูลส่วนตัว ภาพหน้าจอ และขั้นตอนที่ทำให้เกิดปัญหาได้เมื่อผู้เล่นยินยอม

## 8. การทดสอบโหลดและเสถียรภาพ

ก่อนเปิด Demo ต้องมี soak test ที่ปล่อยเกมทำงานต่อเนื่องหลายชั่วโมงในฉากที่มีการโหลดพื้นที่ สร้างและรื้อฐาน เปลี่ยนเวลา บันทึก และเดินทางซ้ำ เพื่อค้นหา memory leak และ state ที่สะสม เมื่อมี Co-op ต้องทำ long-session test พร้อม reconnect, host migration decision, save conflict และการออกเข้าโลกซ้ำ

Dedicated Server หากนำมาใช้ในระยะต่อไปต้องมี server headless build, health check, graceful shutdown, backup checkpoint และการจำกัดจำนวน actor ต่อพื้นที่ การวัดไม่ควรดูเพียงจำนวนผู้เล่นสูงสุด แต่ต้องดู actor, AI, world objects, replication frequency และ bandwidth ต่อผู้เล่นด้วย

## 9. เกณฑ์ Release Gate

ทุก milestone มี gate ที่ชัดเจน หากไม่ผ่านให้ลดขอบเขตหรือแก้ระบบก่อนเพิ่มคอนเทนต์ ตัวอย่างเช่น Vertical Slice จะไม่ผ่านหากผู้เล่นใหม่หลงจนไม่จบ core loop, เฟรมตกจากการเจออีโคจำนวนเป้าหมาย, การเซฟทำให้ไอเทมหาย หรือมีช่องทางสร้างไอเทมซ้ำ แม้ภาพรวมจะดูสวยก็ตาม

| Gate | ต้องพิสูจน์ |
|---|---|
| Prototype Gate | การเคลื่อนที่ ต่อสู้ จับ และ feedback สนุกพอ |
| Vertical Slice Gate | core loop จบได้ แผนที่หนึ่งเขตและฐานทำงาน |
| Co-op Gate | 4 คนเล่นร่วมกัน จัดการ reconnect และ server authority |
| Content Alpha | คอนเทนต์หลักอยู่ครบ ระบบไม่เปลี่ยนโครงสร้างใหญ่ |
| Beta | เหลือบั๊กสำคัญต่ำ มี performance profile และ save migration |
| Release Candidate | ไม่มี blocker, crash rate อยู่ในเกณฑ์ และ build ทำซ้ำได้ |

## 10. Definition of Done สำหรับฟีเจอร์

ฟีเจอร์หนึ่งถือว่าเสร็จเมื่อใช้งานได้ใน Single-player และตามขอบเขต Multiplayer ที่กำหนด มี UI และ feedback ครบ รองรับ save/load มี error handling ผ่าน performance budget มี automated test ที่สำคัญ ผ่าน controller และ accessibility check และมีเอกสารสำหรับ designer ปรับค่าได้

การสร้างฟีเจอร์ที่ยังมี placeholder ทำได้ในช่วงต้น แต่ต้องระบุชัดว่า placeholder มีความเสี่ยงอะไร หากแกนข้อมูลหรือ network contract เปลี่ยนภายหลัง ต้อง migrate ให้ได้ ไม่ปล่อยให้ placeholder กลายเป็นโค้ดถาวรที่ไม่มีเจ้าของ

## References

[1]: https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine "Epic Games — World Partition in Unreal Engine"

[2]: https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine "Epic Games — Nanite Virtualized Geometry Overview"

[3]: https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-global-illumination-and-reflections-in-unreal-engine "Epic Games — Lumen Global Illumination and Reflections"

[4]: https://dev.epicgames.com/documentation/en-us/unreal-engine/iris-replication-system-in-unreal-engine "Epic Games — Iris Replication System in Unreal Engine"
