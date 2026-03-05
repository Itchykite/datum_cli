from typing import Optional, List
import os

from dotenv import load_dotenv
from fastapi import FastAPI
from datetime import datetime
from sqlmodel import SQLModel, Field, Relationship, create_engine, Session, select, join
from sqlalchemy import Column, String
from typing_extensions import Annotated

app = FastAPI()

load_dotenv()

DB_USER = os.getenv("DB_USER")
DB_PASSWORD = os.getenv("DB_PASSWORD")
DB_HOST = os.getenv("DB_HOST")
DB_NAME = os.getenv("DB_NAME")

DATABASE_URL = f"mysql+pymysql://{DB_USER}:{DB_PASSWORD}@{DB_HOST}/{DB_NAME}"
engine = create_engine(DATABASE_URL, echo=True)


class Users(SQLModel, table=True):
    id: Annotated[int, Field(primary_key=True)]
    imie: str
    nazwisko: str
    login: Optional[str] = None
    password: Optional[str] = Field(sa_column=Column("pass", String))
    failed_attempts: int = 0
    lock_until: Optional[datetime] = None

    accounts: List["UserAccounts"] = Relationship(back_populates="user")


class Platforms(SQLModel, table=True):
    id: Annotated[int, Field(primary_key=True)]
    nazwa: str

    accounts: List["UserAccounts"] = Relationship(back_populates="platform")


class UserAccounts(SQLModel, table=True):
    id: Annotated[int, Field(primary_key=True)]
    user_id: Optional[int] = Field(default=None, foreign_key="users.id")
    platform_id: Optional[int] = Field(default=None, foreign_key="platforms.id")
    login: str
    haslo: str
    opis: Optional[str]

    user: Optional[Users] = Relationship(back_populates="accounts")
    platform: Optional[Platforms] = Relationship(back_populates="accounts")


@app.post("/users/")
async def create_user(user: Users):
    with Session(engine) as session:
        session.add(user)
        session.commit()
        session.refresh(user)
        return {
            "id": user.id,
            "imie": user.imie,
            "nazwisko": user.nazwisko,
            "login": user.login,
        }


@app.get("/users")
async def get_users(
    user_id: Optional[int] = None,
    imie: Optional[str] = None,
    nazwisko: Optional[str] = None,
    login: Optional[str] = None,
):
    with Session(engine) as session:
        if user_id is not None:
            user = session.get(Users, user_id)
            if not user:
                return {"error": "User not found"}
            return {
                "id": user.id,
                "imie": user.imie,
                "nazwisko": user.nazwisko,
                "login": user.login,
                "accounts": [
                    {"platform_id": a.platform_id, "login": a.login, "haslo": a.haslo}
                    for a in user.accounts
                ],
            }

        query = select(Users)
        if imie:
            query = query.where(Users.imie == imie)
        if nazwisko:
            query = query.where(Users.nazwisko == nazwisko)
        if login:
            query = query.join(UserAccounts).where(UserAccounts.login == login)

        users = session.exec(query).all()

        result = []
        for user in users:
            result.append(
                {
                    "id": user.id,
                    "imie": user.imie,
                    "nazwisko": user.nazwisko,
                    "login": user.login,
                    "accounts": [
                        {
                            "platform_id": a.platform_id,
                            "login": a.login,
                            "haslo": a.haslo,
                        }
                        for a in user.accounts
                    ],
                }
            )

        return result


@app.delete("/users/{user_id}")
async def delete_user(user_id: int):
    with Session(engine) as session:
        user = session.get(Users, user_id)
        if not user:
            return {"error": "User not found"}
        session.delete(user)
        session.commit()
        return {"message": "User deleted successfully"}


@app.post("/platforms/")
async def create_platform(platform: Platforms):
    with Session(engine) as session:
        session.add(platform)
        session.commit()
        session.refresh(platform)
        return {"id": platform.id, "nazwa": platform.nazwa}


@app.get("/platforms/{platform_id}")
async def get_platform(platform_id: int):
    with Session(engine) as session:
        platform = session.get(Platforms, platform_id)
        if not platform:
            return {"error": "Platform not found"}
        return {
            "id": platform.id,
            "nazwa": platform.nazwa,
            "accounts": [
                {"user_id": a.user_id, "login": a.login, "haslo": a.haslo}
                for a in platform.accounts
            ],
        }


@app.delete("/platforms/{platform_id}")
async def delete_platform(platform_id: int):
    with Session(engine) as session:
        platform = session.get(Platforms, platform_id)
        if not platform:
            return {"error": "Platform not found"}
        session.delete(platform)
        session.commit()
        return {"message": "Platform deleted successfully"}


@app.post("/accounts/")
async def create_account(account: UserAccounts):
    with Session(engine) as session:
        session.add(account)
        session.commit()
        session.refresh(account)
        return {
            "id": account.id,
            "user_id": account.user_id,
            "platform_id": account.platform_id,
            "login": account.login,
            "haslo": account.haslo,
            "opis": account.opis,
        }


@app.get("/accounts/{account_id}")
async def get_account(account_id: int):
    with Session(engine) as session:
        account = session.get(UserAccounts, account_id)
        if not account:
            return {"error": "Account not found"}
        return {
            "id": account.id,
            "user_id": account.user_id,
            "platform_id": account.platform_id,
            "login": account.login,
            "haslo": account.haslo,
            "opis": account.opis,
        }


@app.delete("/accounts/{account_id}")
async def delete_account(account_id: int):
    with Session(engine) as session:
        account = session.get(UserAccounts, account_id)
        if not account:
            return {"error": "Account not found"}
        session.delete(account)
        session.commit()
        return {"message": "Account deleted successfully"}
