
<img width="2160" height="1215" alt="Screen_Holder_Thing_2026-Apr-03_04-33-51AM-000_CustomizedView44042802966_png" src="https://github.com/user-attachments/assets/bde9aa72-69e0-434e-8d6b-256bc0ccb082" />

# Eink_Desk_Console
This is a project I am building to track my projects and show data from Home Assistant and other APIs. I don't really recommend anyone build this, because it is incredibly hyperspecific to my use case. Please use it for help and insporation through.

## APIs Needed
1. Open Weather
2. Home Assistant (Self Hosted)
3. Geoapify
4. Google Apps Script

## Bill Of Materials
| Name                  | Purpose                                                                                                  | Quantity | Total Cost (USD) | Link                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       | Distributor |
|-----------------------|----------------------------------------------------------------------------------------------------------|----------|------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------|
| M2 Bolts              | Connecting everything together                                                                           | 8        | 0.08             |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            | Amazon      |
| USB C Cable           | Power and data to the controller                                                                         | 1        | 4.5              |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            | Anywhere    |
| Xiao ESP32 S3         | Does all the beep booping in this project                                                                | 1        | 16.99            | https://www.amazon.com/ESP32S3-2-4GHz-Dual-core-Supported-Efficiency-Interface/dp/B0BYSB66S5/ref=sr_1_4?crid=2DMEW6LNQJ0UM&dib=eyJ2IjoiMSJ9.c_bkuo_mWg_7Ypo9HgDeWHXlr94Z3KrGQXkatpPpKFUYOt8tTs-5FtFP2ZratDyolK_AsMlXlDq08jeS8dmjOCeESPZINdn33LH0KXWFKzbfJYwD_x_UIcs9MtVQ11pb6k-npcPkTIaYzYuJBLehN_zEn0vzAunbeZdGLsRtjXCed2LiYmMHXxlXVcZcWI9wTGTSb3X0_jaZ-FbRnaVz2Xde2fhZLSSpAuLLLk760UI.dJe9EX2XLN4QLkcp1B6bC2T6pjvV0JQI7MFTXh0r3Kw&dib_tag=se&keywords=xiao%2Besp32%2Bs3&qid=1775531802&sprefix=xiao%2Besp32%2Bs3%2Caps%2C183&sr=8-4&th=1 | Amazon      |
| M2 Heat Set Inserts   | Adding Threads for 3d Prints                                                                             | 8        | 0.24             |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            | Amazon      |
| 7.5inch E-Ink Display | It is the main component in the project. It will show the data I need without using basically any power. | 1        | 75.15            | https://www.amazon.com/waveshare-7-5inch-HAT-Raspberry-Consumption/dp/B075R4QY3L#averageCustomerReviewsAnchor                                                                                                                                                                                                                                                                                                                                                                                                                              | Amazon      |

## Wiring Diagram
<img width="1920" height="1080" alt="Driver Board" src="https://github.com/user-attachments/assets/3e9ae4c8-7791-4cc8-852f-1929ed1c5d37" />

## Assembly Instructions
1. Print all of the pieces needed (2 case, 1 base)
2. Use heat set inserts to add threads to the printed holes (8 holes on the bezel piece)
3. Place the screen within the bezel
4. Carefully place the backing print into the bezel to keep the screen in place
5. Screw bolts to keep the printed pieces together
6. Put the case into the base, and screw it in
7. Connect the electronics to the screen
   
### Assembly Animation

https://github.com/user-attachments/assets/f419b6c7-1fa8-42ba-b154-1aed95d2b339

<img width="2160" height="1215" alt="Screen_Holder_Thing_2026-Apr-03_04-26-18AM-000_CustomizedView27974898632_png" src="https://github.com/user-attachments/assets/e826422e-e0f4-4d48-a9a0-543386c8950e" />

<img width="1916" height="1078" alt="Screen_Case_2026-Apr-02_05-09-52AM-000_CustomizedView14609026417_png" src="https://github.com/user-attachments/assets/c5b644a2-8737-4c1b-ac7d-0765fe9694fe" />
