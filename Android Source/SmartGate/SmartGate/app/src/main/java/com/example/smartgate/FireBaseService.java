package com.example.smartgate;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.util.Log;
import android.view.Gravity;
import android.widget.TextView;
import android.widget.Toast;

import androidx.core.app.NotificationCompat;

import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;

public class FireBaseService extends FirebaseMessagingService {

    //Token Log 정의
    @Override
    public void onNewToken(String token){
        Log.d("SmartGate Log", "새로운 토큰: " + token);
    }

    //FireBase 메시지 수신
    @Override
    public void onMessageReceived(RemoteMessage remoteMessage){
        if(remoteMessage.getNotification() != null){
            Log.d("SmartGate Log", "알림 메시지" + remoteMessage.getNotification().getBody());

                    String m_Body = remoteMessage.getNotification().getBody();
            String m_Title = remoteMessage.getNotification().getTitle();

            Intent intent = new Intent(this, MainActivity.class);
            PendingIntent pendingIntent = PendingIntent.getActivity(this,0, intent, PendingIntent.FLAG_ONE_SHOT);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);

            //Push Alarm 기능 정의
            NotificationCompat.Builder push_Builder = new NotificationCompat.Builder(this)
                    .setContentTitle(m_Title).setContentText(m_Body).setSmallIcon(R.drawable.samsung_logo)
                    .setContentIntent(pendingIntent).setDefaults(Notification.DEFAULT_VIBRATE).setAutoCancel(true);

            //Push Alarm 호출
            NotificationManager push_Alarm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
            push_Alarm.notify(0, push_Builder.build());
        }
    }

    //Toast Message → Static 정의
    public static void setCustomToast(Context context, String msg) {
        TextView m_temp = new TextView(context);
        m_temp.setBackgroundResource(R.color.colorItem);
        m_temp.setPadding(32, 32, 32, 32);
        m_temp.setTextSize(16);
        m_temp.setText(msg);

        final Toast toast = Toast.makeText(context, "", Toast.LENGTH_LONG);
        toast.setGravity(Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM, 0, 48);
        toast.setView(m_temp);
        toast.show();

        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                toast.cancel();
            }
        }, 20000);
    }
}
