package com.arduinoparkingauto

import android.annotation.SuppressLint
import android.content.ContentValues.TAG
import android.graphics.Color
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import androidx.cardview.widget.CardView
import com.google.firebase.FirebaseApp
import com.google.firebase.database.*

class MainActivity : AppCompatActivity() {

    private val database = FirebaseDatabase.getInstance("https://arduinoparkingauto-default-rtdb.asia-southeast1.firebasedatabase.app")
    private val slot1State = database.getReference("/slot1/state")
    private val slot1Card = database.getReference("/slot1/card")
    private val slot2State = database.getReference("/slot2/state")
    private val slot2Card = database.getReference("/slot2/card")

    private lateinit var slot1StateTV : TextView
    private lateinit var slot1CardTV : TextView
    private lateinit var slot2StateTV : TextView
    private lateinit var slot2CardTV : TextView

    private lateinit var resetButton1: Button
    private lateinit var resetButton2: Button

    private lateinit var adminButton1: Button

    private lateinit var slot1CardView: CardView
    private lateinit var slot2CardView: CardView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        slot1StateTV = findViewById(R.id.statusSlot1)
        slot1CardTV = findViewById(R.id.cardSlot1)
        slot2StateTV = findViewById(R.id.statusSlot2)
        slot2CardTV = findViewById(R.id.cardSlot2)

        resetButton1 = findViewById(R.id.btnSlot1)
        resetButton2 = findViewById(R.id.btnSlot2)

        adminButton1 = findViewById(R.id.admSlot1)

        slot1CardView = findViewById(R.id.slot1CardView)
        slot2CardView = findViewById(R.id.slot2CardView)

        FirebaseApp.initializeApp(this)

        resetButton1.setOnClickListener {
            slot1State.setValue("WAIT_GATE")
            slot1Card.setValue("empty")
        }

        resetButton2.setOnClickListener {
            slot2State.setValue("WAIT_GATE")
            slot2Card.setValue("empty")
        }

        adminButton1.setOnClickListener {
            slot1State.setValue("PARK_CAR")
        }

        // Listen for changes in /slot1/state
        slot1State.addValueEventListener(object : ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                val slot1StateValue = dataSnapshot.getValue(String::class.java).toString()
                slot1StateTV.text = "Status : $slot1StateValue"
                Log.d("autopark", "Slot1 : ${slot1StateTV.text}")

                if (slot1StateValue == "LEAVE_CAR") {
                    slot1CardView.setCardBackgroundColor(Color.parseColor("#61ff76"))
                    resetButton1.visibility = View.VISIBLE
                    adminButton1.visibility = View.GONE
                    Log.d("autopark", "if worked")
                } else if (slot1StateValue.startsWith("WARNING")) {
                    slot1CardView.setCardBackgroundColor(Color.parseColor("#ffe98f"))
                    resetButton1.visibility = View.VISIBLE
                    adminButton1.visibility = View.GONE
                } else if (slot1StateValue == "ADMIN_RESET") {
                    slot1CardView.setCardBackgroundColor(Color.parseColor("#f57373"))
                    resetButton1.visibility = View.GONE
                    adminButton1.visibility = View.VISIBLE
                } else {
                    slot1CardView.setCardBackgroundColor(Color.WHITE)
                    resetButton1.visibility = View.VISIBLE
                    adminButton1.visibility = View.GONE
                }
            }
            override fun onCancelled(error: DatabaseError) {
                Log.w(TAG, "Failed to read value.", error.toException())
            }
        })

        // Listen for changes in /slot1/card
        slot1Card.addValueEventListener(object : ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                slot1CardTV.text = "Card ID : " + dataSnapshot.getValue(String::class.java).toString()
            }

            override fun onCancelled(error: DatabaseError) {
                Log.w(TAG, "Failed to read value.", error.toException())
            }
        })

        // Listen for changes in /slot2/state
        slot2State.addValueEventListener(object : ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                slot2StateTV.text = "Status : " + dataSnapshot.getValue(String::class.java).toString()

                if (slot2StateTV.text.equals("LEAVE_CAR")){
                    slot2CardView.setCardBackgroundColor(Color.parseColor("#61ff76"))
                } else if (slot2StateTV.text.startsWith("WARNING")){
                    slot2CardView.setCardBackgroundColor(Color.parseColor("#ffe98f"))
                } else if (slot2StateTV.text.equals("ADMIN_RESET")){
                    slot2CardView.setCardBackgroundColor(Color.parseColor("#f57373"))
                } else {
                    slot2CardView.setCardBackgroundColor(Color.WHITE)
                }
            }

            override fun onCancelled(error: DatabaseError) {
                Log.w(TAG, "Failed to read value.", error.toException())
            }
        })

        // Listen for changes in /slot2/card
        slot2Card.addValueEventListener(object : ValueEventListener {
            override fun onDataChange(dataSnapshot: DataSnapshot) {
                slot2CardTV.text = "Card ID : " + dataSnapshot.getValue(String::class.java).toString()
            }

            override fun onCancelled(error: DatabaseError) {
                Log.w(TAG, "Failed to read value.", error.toException())
            }
        })

    }
}
