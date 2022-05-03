#include <Arduino.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>

#define WIFI_SSID "nom_point_d'accès"
#define WIFI_PASSWORD "mot de passe "
#define AUTHOR_EMAIL "adresse@mail "   // coordonnées de l'expéditeur 
#define AUTHOR_PASSWORD "mot de passe"


#define SMTP_HOST "smtp.gmail.com"

/** les ports de connexion 
 * 465 pour le port  ssl
 * 587  pour le port de  tls
*/
#define SMTP_PORT 465


// coordonnées du récepteur
#define RECIPIENT_EMAIL "adresse@mail" //adresse destinataire

//objet smtp

SMTPSession smtp;

// fonction de retour 
void smtpCallback(SMTP_Status status);

void setup(){
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connexion...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("WiFi connectée.");
  Serial.println("Adresse IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  if (!SPIFFS.begin(true)) {
    Serial.println("une erreur est survenue lors de la lecture dans la mémoire flash");
  }
  else{
    Serial.println("lecture réussie");
  }

  /** activation du débogage dans le port série
   * pas de debogage  -> 0
   * debogage basique -> 1
  */
  smtp.debug(1);

  /* appel de la fonction pour obtenir le resultat */
  smtp.callback(smtpCallback);

  /* initilisation de la session d'envoi des données  */
  ESP_Mail_Session session;

  /* definitions des paramètres de la session  */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "mydomain.net";

  //configuration du message SMTP 
  SMTP_Message message;

  // nécessaire pour autoriser l'envoi des fichiers volumineux 
  message.enable.chunking = true;

  // modifier l'entête du message 
  message.sender.name = "ESP32 notification";
  message.sender.email = AUTHOR_EMAIL;

// définir le sujet du message 
  message.subject = " Alerte de sécurité sur votre système domotique";
  message.addRecipient("Leo", RECIPIENT_EMAIL);

  //  définition du corps du message en format html et text
  String htmlMsg = "Ce message contient des insrtructions à suivre en pièces jointes";
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /******  définition des pièces jointes *******/
  SMTP_Attachment pj;

  
  pj.descr.filename = "alert.png";   // non du fichier
  pj.descr.mime = "image/png"; //format du fichier
  pj.file.path = "/alert.png";  // l'emplacement du fichier 
  pj.file.storage_type = esp_mail_file_storage_type_flash;  // la nature du stockage
  pj.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;  // l'encodage 

  // joindre la pièce jointe
  message.addAttachment(pj);

  message.resetAttachItem(pj);
  //2e pièce jointe
  pj.descr.filename = "consigne.txt";
  pj.descr.mime = "text/plain";
  pj.file.path = "/consigne.txt";
  pj.file.storage_type = esp_mail_file_storage_type_flash;
  pj.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  
  message.addAttachment(pj);

  //  connection au serveur avec la session configurée
  if (!smtp.connect(&session))
    return;

  // envoi de l'email et fermeture de la session 
  if (!MailClient.sendMail(&smtp, &message, true))
    Serial.println("Erreur lors de l'envoi, " + smtp.errorReason());
}

void loop()
{
}

// définition de la fonction de retour pour connaitre le statut de l'envoi 
void smtpCallback(SMTP_Status status){
  
  Serial.println(status.info());

  // affichage du resultat
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message bien réçu: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("message non envoyé: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* obtenir le resultat */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "succes" : "echec");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}