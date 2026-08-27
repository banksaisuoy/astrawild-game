# ASTRAWILD — 3D Asset, Map, Audio & License Plan

## สถานะปัจจุบันที่ตรวจจาก repository

ณ audit รุ่น `227c8a1` repository มี C++ source, Config, เอกสาร, สคริปต์ validation และ architecture diagram เท่านั้น ภายใต้ `Content/ASTRAWILD` มีเพียง `.gitkeep` ไม่มีไฟล์ `.uasset`, `.umap`, `.fbx`, `.glb`, texture, animation, sound, character mesh หรือ environment mesh ดังนั้น **โมเดล 3D และแผนที่ 3D ยังไม่ได้ทำ** และเกมยังไม่สามารถเปิดแล้วเห็นโลกหรือกด Play เป็น Vertical Slice ได้จนกว่า Antigravity จะสร้างหรือ import asset ใน Unreal Editor

## เป้าหมาย asset สำหรับ Vertical Slice

ไม่ควรเริ่มจากการทำโลกขนาดใหญ่หรืออีโคจำนวนมาก ให้ทำชุดทดลองที่เล็กแต่ครบวงจร โดยใช้ placeholder ที่เปลี่ยนภายหลังได้ง่าย

| Asset group | จำนวนรุ่นทดลอง | สถานะตอน audit | ผู้รับผิดชอบถัดไป |
|---|---:|---|---|
| Player mesh/animation | 1 | ยังไม่มี | Antigravity import/create ใน Unreal |
| Echo creature | 3 | ยังไม่มี | Antigravity ใช้ placeholder ก่อน แล้วแทนด้วย mesh ที่มี license |
| Enemy/test target | 1 | ยังไม่มี | Antigravity สร้างจาก primitive หรือ licensed placeholder |
| Resource props | 3–5 | ยังไม่มี | Antigravity ใช้ rocks/foliage/primitive |
| Rest point/building | 1 modular kit | ยังไม่มี | Antigravity สร้าง Blueprint + mesh placeholder |
| Prototype map | 1 พื้นที่เล็ก | ยังไม่มี | Antigravity สร้าง `L_Prototype.umap` |
| UI icons | 5–10 | ยังไม่มี | ใช้ icon placeholder ก่อน |
| VFX | 3–5 | ยังไม่มี | Niagara primitive/basic materials |
| Sound | 10–20 cue | ยังไม่มี | ใช้เสียง placeholder ที่มี license หรือทำ procedural |

## แหล่ง asset ที่แนะนำ

### Fab / Unreal ecosystem

ใช้ Fab สำหรับ environment kit, rocks, modular ruins, foliage, materials, animations หรือ character packs ที่ระบุ license ชัดเจน Fab Standard License ระบุว่าสามารถใช้แบบส่วนตัวหรือเชิงพาณิชย์ ปรับแก้เพื่อรวมใน Project และแชร์ asset กับผู้ร่วมโครงการได้ แต่ไม่อนุญาตให้ขายหรือแจก asset แบบแยกเดี่ยว [1] การที่เกมนี้เล่นกับเพื่อนและไม่ขายไม่ได้แปลว่า asset ทุกชนิดใช้ได้โดยไม่มีเงื่อนไข จึงต้องเก็บชื่อผู้สร้าง ลิงก์หน้า asset วันที่ดาวน์โหลด และ license ไว้ใน `Docs/ThirdPartyLicenses.md`

### Kenney

ใช้ Kenney สำหรับ placeholder props, UI, icons และสิ่งของทั่วไปที่ต้องการให้ Antigravity ประกอบเกมได้เร็ว Kenney ระบุว่า asset บนหน้าสินค้าเป็น public domain/CC0 ใช้ในโปรเจกต์ส่วนตัวหรือเชิงพาณิชย์ได้ และไม่บังคับ attribution [2] อย่างไรก็ตาม ให้เก็บลิงก์ของชุด asset และไฟล์ license ไว้เพื่อการตรวจสอบย้อนหลัง

### Poly Haven

ใช้ Poly Haven สำหรับ HDRI, PBR textures, rocks, ground materials และ props ที่เหมาะกับฉากต้นแบบ Poly Haven ระบุว่า asset เป็น CC0 และใช้ได้ทุกวัตถุประสงค์ แต่เนื้อหาเว็บไซต์ เช่น logo, ภาพตัวอย่าง และข้อความ ไม่ได้รวมอยู่ในสิทธิ์ CC0 เดียวกัน [3] [4] จึงดาวน์โหลดเฉพาะไฟล์ asset จากหน้าของ asset ไม่คัดลอกภาพตัวอย่างมาใช้ในเกม

### Asset ที่ต้องระวัง

Sketchfab, CGTrader, Mixamo, GitHub, YouTube และเว็บไซต์รวม asset มี license แตกต่างกันเป็นรายไฟล์ ห้ามใช้เพียงเพราะค้นเจอหรือดาวน์โหลดได้ การใช้โมเดลที่มีชื่อ ตัวละคร หรือ texture จาก Pokémon, ARK, Palworld, Nintendo, Pocketpair หรือเกมอื่นโดยตรงไม่ควรทำ แม้เล่นกันเอง เพราะเสี่ยงละเมิดสิทธิ์และทำให้เกมไม่มีเอกลักษณ์ของตัวเอง

## แผนทำโมเดล Echo

สำหรับ Vertical Slice ให้ Antigravity เริ่มด้วย placeholder mesh ที่สร้างจาก primitive เพื่อทดสอบระบบก่อน จากนั้นเลือกหนึ่งในสามทางต่อไปนี้ตามเวลาและเครื่องมือของทีม:

| ทางเลือก | ใช้เมื่อ | ข้อดี | ข้อจำกัด |
|---|---|---|---|
| Licensed asset จาก Fab | ต้องการผลเร็วและมีงบ asset | rig/mesh พร้อมกว่า | ต้องตรวจ license และอาจไม่เข้ากับสไตล์ |
| CC0/placeholder จากแหล่งเปิด | ต้องการทดสอบระบบและเล่นกับเพื่อน | ต้นทุนต่ำ ใช้เร็ว | ต้องทำให้รูปลักษณ์มีเอกลักษณ์เอง |
| สร้างต้นฉบับใน Blender/เครื่องมือสร้างภาพ 3D | ต้องการตัวตนของ ASTRAWILD | ควบคุมดีไซน์และสิทธิ์ได้ | ใช้เวลา rig, skin, animation และ optimization |

อีโคแต่ละตัวต้องมี silhouette ต่างกัน บทบาทต่างกัน และมี collision/animation ที่ใช้ได้จากกล้อง third-person ไม่ควรทำ model high-poly อย่างเดียวแล้วถือว่าเสร็จ ต้องมี skeleton, physics asset, sockets, animation set, material, LOD/fallback, collision, Data Asset และ test Blueprint ครบ

## แผนทำแผนที่ 3D

แผนที่แรกไม่ต้องเป็น Open World ใหญ่ ให้สร้างพื้นที่ทดลองประมาณหนึ่ง hub, หนึ่งเส้นทางทรัพยากร, หนึ่งจุดอันตราย, หนึ่ง landmark, หนึ่ง rest point และหนึ่งจุดทดสอบ combat บน landscape หรือ modular floor ขนาดเล็ก ใช้ World Partition ได้หากต้องการทดสอบ pipeline แต่ไม่จำเป็นต้องสร้างหลายเซลล์จนเสียเวลา

`L_Prototype.umap` ต้องมีอย่างน้อย player start, GameMode, player Blueprint, resource nodes, three Echo actors, rest point, lighting, collision/floor, navigation data เมื่อ AI พร้อม และ debug HUD พื้นที่นี้ต้องทำให้ผู้เล่นเล่น core loop จบได้ใน 10–20 นาที ไม่ใช่แผนที่สวยที่ไม่มีสิ่งให้ทำ

## Asset manifest ที่ต้องเพิ่มหลัง Antigravity ทำงาน

สร้างไฟล์ `Docs/ThirdPartyLicenses.md` และบันทึกข้อมูลทุก asset ตามตารางนี้:

| Asset ID | ชื่อไฟล์/ชุด | ประเภท | URL | License | วันที่ | ใช้ในเกมตรงไหน |
|---|---|---|---|---|---|---|
| `ENV_PROTO_ROCKS_01` |  | rock/prop |  |  |  | prototype map |
| `ENV_PROTO_GROUND_01` |  | material/texture |  |  |  | landscape |
| `ECHO_EXPLORER_01` |  | creature mesh |  |  |  | Echo Explorer |
| `ECHO_COMBAT_01` |  | creature mesh |  |  |  | Echo Combat |
| `ECHO_BASE_01` |  | creature mesh |  |  |  | Echo Base |
| `SFX_UI_01` |  | sound |  |  |  | UI/capture |

ห้าม commit raw asset pack ที่ไม่จำเป็นหรือไฟล์ที่มีขนาดใหญ่โดยไม่ใช้ Git LFS/Google Drive ตามนโยบาย `Docs/ASSET_STORAGE.md` หาก asset เป็นของ Fab ที่ผูกกับบัญชีผู้ใช้ ให้เก็บข้อมูลการติดตั้งและ license record ไม่อัปโหลดไฟล์ที่อนุญาตเฉพาะผู้ซื้อไปใน public repository

## ลำดับงานที่ทำให้ Antigravity เร็วที่สุด

1. Compile C++ ก่อน โดยยังใช้ primitive และ default material
2. สร้าง `BP_AstrawildPlayer`, `BP_Echo_*`, `BP_ResourceNode` และ `BP_RestPoint`
3. สร้าง Data Assets ที่ใช้ ID ตรงกับ C++
4. สร้าง `L_Prototype.umap` ให้ core loop เล่นได้
5. ทดสอบ Save/Load และแก้ API error
6. ค่อยเลือก asset pack ที่มี license และแทน placeholder ทีละกลุ่ม
7. ทำ material, animation, VFX และ audio polish หลังระบบผ่าน

## References

[1]: https://www.fab.com/eula?lang "Fab — Fab Standard License"

[2]: https://kenney.nl/support "Kenney — Common questions and game asset license"

[3]: https://polyhaven.com/license "Poly Haven — Asset License"

[4]: https://docs.polyhaven.com/en/faq "Poly Haven Wiki — FAQ"
